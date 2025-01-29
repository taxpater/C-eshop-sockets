// Addition of the necessary packages / libraries to the program.
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "func.h"

// The main function that executes all the processes of the e-shop.
int main(void) {
  // Initialization of the i, j and k counters along with the fd1 and fd2 tables that will be used for the two pipes connecting 
  // the clients and the server.
  int i = 1, j, k, response;
  char buffer[512];
  product catalog[CATALOG_SIZE];
  InitialiseCatalog(catalog);
  
  while(i <= CLIENT_AMOUNT) {
    // The seed for the rand() function is changed with every new client to decrease the chances of multiple duplicate requests of the same product.
    srand(time(NULL));

    // Initialization of the socket and the necessary variables that will be used for the client-server communication.
    struct sockaddr_un sa;
    (void)unlink(SOCKETNAME);
    strcpy(sa.sun_path, SOCKETNAME);
    sa.sun_family = AF_UNIX;
    int fd_skt, fd_client; 
    int writeStatus;
    ssize_t readStatus;
    
    int customerTotal = 0;
    printf("--------------------------------\nClient No.%d: \n", i);
    pid_t clientPid = fork();
    
    // If fork() returns any negative integer, it meants that we have encountered a fatal error and the program should quit.
    if (clientPid < 0) {
      printf("Error Number : %d\n", errno);
      perror("Error Description");
      return (-2); 
    }

    // Client Process.
    if (clientPid == 0) {
      if ((fd_skt = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("Message from socket [client] : ");
        exit(-1);
      }
          
      // Since client and server run concurrently, it is possible for the client
      // to call connect before the creation of the socket file by the server (ENOENT).
      // In this case the function sleeps for a second and tries again. If something
      // else goes wrong, the function fails.
      while (connect(fd_skt, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
        if (errno == ENOENT) {
          sleep(1); continue; }
        else {
          perror("Message from connect [client]");
          exit(-2); }}
          
      int productNo;
      
      // The loop that allows the client to send the "ORDER_AMOUNT" orders to the server.
      for (j = 1; j <= ORDER_AMOUNT; j++) {
        productNo = rand() % CATALOG_SIZE;
        
        RequestNewOrder(fd_skt, productNo);
        sleep(1); // Order processing time
        
        // Reading the response of the server.
        readStatus = read(fd_skt, buffer, sizeof(buffer));
        if (readStatus==-1) {
          perror("Message from read [client]");
          exit(-4);
        }
        
        // Printing the response of the server.
        printf("%s", buffer);
      }
      // Closing the file descriptor that the client was using and exiting the client process.
      close(fd_skt);
      exit(0);
    }
    // Server Process
    else {
      // Preparing the socket on the server's side to ensure that it is ready to accept the orders from the clients.
      if ((fd_skt = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("Message from socket [server]");
        exit(-1);
      }    
             
      if (bind(fd_skt, (struct sockaddr *) &sa, sizeof(sa))){
        perror("Message from bind [server]");
        exit(-2);
      }              
      if (listen(fd_skt, 2)){
        perror("Message from listen [server]");
        exit(-3);
      }
      if ((fd_client = accept(fd_skt, NULL, 0)) < 0) {
        perror("Message from accept [server]"); 
        exit(-4);
      } 
      
      // The loop that allows the server to handle the 10 orders that each client sends to it.
      for (k = 1; k <= ORDER_AMOUNT; k++) {
        readStatus = read(fd_client, &response, sizeof(int));
        if (readStatus==-1) {
          perror("Message from read [client]");
          exit(-4);
        }
        
        // The logic that decides what the response to the client is,along with the appropriate adjustment of the statistics variables.
        if (catalog[response].item_count >= 1) {
          customerTotal = customerTotal + catalog[response].price;
          catalog[response].item_count--;
          catalog[response].successfulPurchases++;
          snprintf(buffer, sizeof(buffer), "Purchase of item No.%d was successful! Your charge is: %d€.\n", response, catalog[response].price);
        } else {
          catalog[response].failedPurchases++;
          snprintf(buffer, sizeof(buffer), "There isn't enough stock of item No.%d! Your order has been canceled.\n", response);
        }
        
        // Using the SendOrderStatus() function to send the server's reply to the order to the client.
        SendOrderStatus(fd_client, buffer, sizeof(buffer));
        // Order cooldown
        sleep(1); 
      }
      printf("\nThe total for client No.%d is: %d\n", i, customerTotal);
      
      // "Freezing" the server to ensure that the client has exited before proceeding with handing the next client's orders. 
      waitpid(clientPid, NULL, 0);
      
      // Closing the rest of the socket's ends since they are no longer needed.
      close(fd_skt);
      close(fd_client);
    }
    sleep(1);
    i++;
  }
  
  // Calling the relevant function to display the final report for the e-shop.
  StatisticsReporting(catalog);
  // After we are done with the usage of the socket, we unlink() it to ensure that the allocated resources are freed and there
  // won't be any conflicts when running the program again.
  unlink(SOCKETNAME);
  return 0;
}
