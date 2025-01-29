// Addition of the necessary packages / libraries to the program.
#include "func.h"
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

// The function that sends an order to the server.
void RequestNewOrder(int fd_skt, int chosenProduct) {
  int writeStatus;
  
  // Sending the order to the server.
  writeStatus = write(fd_skt, &chosenProduct, sizeof(int));
  if (writeStatus == -1) {
    perror("Message from write [client]");
    exit(-3);
  }
}

// The function that replies to the client after an order is received and processed.
void SendOrderStatus(int fd_client, char buffer[], int bufferSize) {
  int writeStatus;
  
  writeStatus = write(fd_client, buffer, bufferSize);
  if (writeStatus == -1) {
    perror("Message from write [client]");
    exit(-3);
  }
}

