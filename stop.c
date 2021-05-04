#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "simulator.h"

int main() {
  int                 clientSocket, addrSize, bytesReceived;
  struct sockaddr_in  clientAddr;
  char                buffer[80];

  // Register with the server
  clientSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (clientSocket < 0) {
    printf("*** CLIENT ERROR: Could open socket.\n");
    exit(-1);
  }
  memset(&clientAddr, 0, sizeof(clientAddr));
  clientAddr.sin_family = AF_INET;
  clientAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
  clientAddr.sin_port = htons((unsigned short) SERVER_PORT);

  //Send the command string to the server 
  buffer[0] = STOP;
  printf("CLIENT: Sending STOP \"%d\" to server.\n", buffer[0]);
  sendto(clientSocket, buffer, strlen(buffer), 0, (struct sockaddr *) &clientAddr, sizeof(clientAddr));

  close(clientSocket); 
  printf("CLIENT: Shutting down.\n");
}
