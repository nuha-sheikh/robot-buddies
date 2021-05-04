#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "simulator.h"




// This is the main function that simulates the "life" of the robot
// The code will exit whenever the robot fails to communicate with the server
int main() {
  int                 clientSocket, addrSize, bytesReceived;
  struct sockaddr_in  clientAddr;
  unsigned char                buffer[80];
  int newCollision = 0;
  int tempSign = 0;
  int c=0;
  int flag = 0;

  // Set up the random seed
  srand(time(NULL));
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

  // Send register command to server.
  buffer[0] = REGISTER;
  sendto(clientSocket, buffer, strlen(buffer), 0, (struct sockaddr *) &clientAddr, sizeof(clientAddr));
  //Get back response data and store it.   If denied registration, then quit.
  bytesReceived = recvfrom(clientSocket, buffer, 80, 0, (struct sockaddr *) &clientAddr, &addrSize);
  //If client received: NOT_OK
  if(buffer[0] == NOT_OK){
    printf("CLIENT: Error! Got back response NOT_OK \"%d\" from server.\n", buffer[0]);
    close(clientSocket);
    flag = 1;
    printf("CLIENT: Shutting down.\n");
   }else{
    //If client received: OK... continue
    float x = (float)((buffer[2]*256)+buffer[3]);
    float y = (float)((buffer[4]*256)+buffer[5]);
    float direction = (float)((buffer[6]*256)+buffer[7]);
    int sign = buffer[8];
    if(sign == 1){
       //if sign in 1, it will be a negative direction
       direction = -(direction);
    }
   }

  // Go into an infinite loop exhibiting the robot behavior
  while (flag == 0) {
    // Check if can move forward
    buffer[0] = CHECK_COLLISION;
    sendto(clientSocket, buffer, sizeof(buffer), 0, (struct sockaddr *) &clientAddr, sizeof(clientAddr));

    // Get response from server.
    bytesReceived = recvfrom(clientSocket, buffer, 80, 0, (struct sockaddr *) &clientAddr, &addrSize);

    float x = (float)((buffer[2]*256)+buffer[3]);
    float y = (float)((buffer[4]*256)+buffer[5]);
    float direction = (float)((buffer[6]*256)+buffer[7]);
    if(buffer[8] == 1){
       //if sign in 1, it will be a negative direction
       direction = -(direction);
    }

    // If ok, move forward
    if(buffer[0] == OK){
      //Calculate new location
      float newX = x + ROBOT_SPEED * cos(direction*(PI/180));
      float newY = y + ROBOT_SPEED * sin(direction*(PI/180));
      if(buffer[8] == 1){
          //if sign in 1, it will be a negative direction
          direction = -(direction);
      }
      //Update location in Buffer
          buffer[2] = (char)((int)round(newX) / 256);
    	  buffer[3] = (char)((int)round(newX) % 256);
    	  buffer[4] = (char)((int)round(newY) / 256);
    	  buffer[5] = (char)((int)round(newY) % 256);
      newCollision = 0;
    }

    else if(buffer[0] == LOST_CONTACT){
    	break;
    }
    // Otherwise, we could not move forward, so make a turn.
    // If we were turning from the last time we collided, keep
    // turning in the same direction, otherwise choose a random
    // direction to start turning. (direction is only changed)
    else{
      //New Collision
      if(newCollision == 0){
         newCollision = 1;
         tempSign = rand() % 2;

         if(tempSign == 1){
             direction = direction - ROBOT_TURN_ANGLE;
             if(direction < -180){
             	direction = direction + 360;
             }
         }else{
            direction = direction + ROBOT_TURN_ANGLE;
             if(direction > 180){
             	direction = direction - 360;
             }
         }
         buffer[6] = (char)((int)abs(round(direction)) / 256);
         buffer[7] = (char)((int)abs(round(direction)) % 256);
         buffer[8] = (direction < 0);
      }else{
      //Old Collision
        if(tempSign == 1){
             direction = direction - ROBOT_TURN_ANGLE;
             if(direction < -180){
             	direction = direction + 360;
             }
         }else{
            direction = direction + ROBOT_TURN_ANGLE;
             if(direction > 180){
             	direction = direction - 360;
             }
         }
         buffer[6] = (char)((int)abs(round(direction)) / 256);
         buffer[7] = (char)((int)abs(round(direction)) % 256);
         buffer[8] = (direction < 0);
      }

    }

    // Send update to server
    buffer[0] = STATUS_UPDATE;
    //sends latest location, direction to server
    sendto(clientSocket, buffer, sizeof(buffer), 0, (struct sockaddr *) &clientAddr, sizeof(clientAddr));

    usleep(10000);

  }
  printf("ROBOT: Shutting down.\n");
  close(clientSocket);
}
