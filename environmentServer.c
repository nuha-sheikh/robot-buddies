#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "simulator.h"


Environment    environment;  // The environment that contains all the robots
int checkCollision(float newX, float newY, float direction, int id, void *);

// Handle client requests coming in through the server socket.  This code should run
// indefinitiely.  It should repeatedly grab an incoming messages and process them.
// The requests that may be handled are STOP, REGISTER, CHECK_COLLISION and STATUS_UPDATE.
// Upon receiving a STOP request, the server should get ready to shut down but must
// first wait until all robot clients have been informed of the shutdown.   Then it
// should exit gracefully.
void *handleIncomingRequests(void *e) {
	Environment *env = e;
	char   online = 1;
	env->numRobots = 0;
	//Variables for the Server
	int                 serverSocket;
	struct sockaddr_in  serverAddr, clientAddr;
	int                 status, addrSize, bytesReceived;
	fd_set              readfds, writefds;
	unsigned char                buffer[80];
	int count = 0;

  	// Initialize the server
  	// Create the server socket
	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (serverSocket < 0) {
	   printf("*** SERVER ERROR: Could not open socket.\n");
	   exit(-1);
	}
	// Setup the server address
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons((unsigned short) SERVER_PORT);
	// Bind the server socket
	status = bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
	if (status < 0) {
	  printf("*** SERVER ERROR: Could not bind socket.\n");
	  exit(-1);
	}

  	// Wait for clients now
	while (online) {
		FD_ZERO(&readfds);
		FD_SET(serverSocket, &readfds);
		FD_ZERO(&writefds);
		FD_SET(serverSocket, &writefds);
		status = select(FD_SETSIZE, &readfds, &writefds, NULL, NULL);
		if (status == 0) {
		  // Timeout occurred, no client ready
		}else if (status < 0) {
		  printf("*** SERVER ERROR: Could not select socket.\n");
		  exit(-1);
		}else {
		  //The server is receiving a request/signal
		  //Signals could either be: STOP, REGISTER, CHECK_COLLISION & STATUS_UPDATE
		  //buffer: 0-Signal, 1-Position, 2-3: X coords, 4-5: Y coords, 6-7: direction coords, 8: magnitude
		  addrSize = sizeof(clientAddr);
    		  bytesReceived = recvfrom(serverSocket, buffer, 80, 0, (struct sockaddr *) &clientAddr, &addrSize);

    		  //SIGNAL: STOP
		  if(buffer[0] == STOP){
		     printf("Got response\n");
		    env->shutDown = 1;
		  }
		  //SIGNAL: REGISTER
		  else if(buffer[0] == REGISTER){

		   if(env->numRobots == MAX_ROBOTS){
		   	//Will send NOT_OK back, as reached max amount
		   	buffer[0] = NOT_OK;
		   }else{
		    //Will send OK back and (id, x, y, direction, direction sign)
		    buffer[0] = OK;
		    int check = 0;
		    float x = 0;
		    float y = 0;
		    float direction = 0;
		    int sign = 0;
		    int flag = 0;

		    while(check == 0){
		      x = (rand() % (ENV_SIZE - ROBOT_RADIUS)) + (ROBOT_RADIUS);
		      y = (rand() % (ENV_SIZE - ROBOT_RADIUS)) + (ROBOT_RADIUS);
		      direction = (int)(rand()/(double)RAND_MAX*180);
		      sign = (int)(rand()/(double)RAND_MAX*1);
		      if(sign == 1){
		    	  //if sign in 1, it will be a negative direction
		    	  direction = direction * -1;
		      }
		      flag = 0;

		      for(int i=0; i < env->numRobots; i++){
		          if( (env->robots[i].x == x) && (env->robots[i].y == y) ){
		             flag = 1;
		             break;
		          }
		      }

		      if(flag == 0){
		        check = 1;
		      }

		    }

		    //Setting up Robot in Environment
		    env->robots[env->numRobots].x = x;
		    env->robots[env->numRobots].y = y;
		    env->robots[env->numRobots].direction = direction;

		    //Sending back data to Client
		    buffer[1] = env->numRobots;
		    buffer[2] = (char)((int)round(x) / 256);
		    buffer[3] = (char)((int)round(x) % 256);
		    buffer[4] = (char)((int)round(y) / 256);
		    buffer[5] = (char)((int)round(y) % 256);
		    buffer[6] = (char)((int)round(direction) / 256);
		    buffer[7] = (char)((int)round(direction) % 256);
		    buffer[8] = sign;

		    env->numRobots++;
		    }
		    sendto(serverSocket, buffer, sizeof(buffer), 0, (struct sockaddr *) &clientAddr, sizeof(clientAddr));
		  }

		  //SIGNAL:CHECK_COLLISION
		  else if(buffer[0] == CHECK_COLLISION){
		      usleep(10000/(env->numRobots) + 1);
		      //Command, ID, location and direction of robot
		     float x = (float)((buffer[2]*256)+buffer[3]);
    		     float y = (float)((buffer[4]*256)+buffer[5]);
                     float direction = (float)((buffer[6]*256)+buffer[7]);
      		      if(buffer[8] == 1){
                             //if sign in 1, it will be a negative direction
                            direction = -(direction);
                      }

                      buffer[0] = OK;
                      if(env->shutDown == 1){
                      	buffer[0] = LOST_CONTACT;
                      	sendto(serverSocket, buffer, sizeof(buffer[0]), 0, (struct sockaddr *) &clientAddr, sizeof(clientAddr));
                      	env->numRobots--;
                      	if(env->numRobots == 0){
                      	 break;
                      	}
                      }else{

                      float newX = x + ROBOT_SPEED * cos(direction*(PI/180));
      		      float newY = y + ROBOT_SPEED * sin(direction*(PI/180));

                      if(checkCollision(newX, newY, direction, buffer[2], &env) == 1){
                          buffer[0] = OK;
                      }else if(checkCollision(newX, newY, direction, buffer[2], &env) == 0){
                        //Boundary collision
                        buffer[0] = NOT_OK_BOUNDARY;
                      }else{
                        //Robot collision
                        buffer[0] = NOT_OK_COLLIDE;
                      }
                      sendto(serverSocket, buffer, sizeof(buffer[0]), 0, (struct sockaddr *) &clientAddr, sizeof(clientAddr));
                      }

    	   	  }

    	   	  else if(buffer[0] == STATUS_UPDATE){

    	   	        float x = (float)((buffer[2]*256)+buffer[3]);
    			float y = (float)((buffer[4]*256)+buffer[5]);
    			float d = (float)((buffer[6]*256)+buffer[7]);
    			if (buffer[8] == 1){
    				d = -(d);
    			}
		      	env->robots[buffer[2]].x = x;
		      	env->robots[buffer[2]].y = y;
		      	env->robots[buffer[2]].direction = d;

    	   	  }
    	   	  else{
    	   	    printf("Unknown Request\n");
    	   	  }
		  }
  	}

  	printf("SERVER: Shutting down.\n");
  	close(serverSocket);
  	pthread_exit(NULL);
}

int checkCollision(float x, float y, float direction, int id, void *env){
   int flag = 1;
   float gdistance = 0;
   Environment *e = env;
   //check if collisions with other robots
   if(e->numRobots > 1){
      for( int i = 0; i< e->numRobots; i++ ){
          if(i != id ){
            float x2 = e->robots[i].x;
            float y2 = e->robots[i].y;
            gdistance = ((x2-x)*(x2-x))+((y2-y)*(y2-y));
            if( sqrt(gdistance) <= (2*ROBOT_RADIUS) ){
               flag = 2;
            }

          }
      }

   }
   //check if collision with boundaries
   if( x-ROBOT_RADIUS <= 0 || x+ROBOT_RADIUS >=  ENV_SIZE){
    	flag = 0;
    }
   if( y-ROBOT_RADIUS <= 0 || y+ROBOT_RADIUS >=  ENV_SIZE){
    	flag = 0;
   }

   if(flag == 0){
      return 0;
   }else if(flag == 2){
      return 2;
   }

   return 1;
}

int main() {
	environment.shutDown = 0;
	srand(time(NULL));
	environment.shutDown = 0;
	srand(time(NULL));

	pthread_t t1, t2;
	pthread_create(&t1, NULL, handleIncomingRequests, &environment);
	pthread_create(&t2, NULL, redraw, &environment);

	// Spawn an infinite loop to handle incoming requests and update the display
	// Wait for the update and draw threads to complete
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	return 0;
}
