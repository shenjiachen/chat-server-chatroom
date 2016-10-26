#include <stdio.h>
#include <stdlib.h>
#include<stdint.h>
#include<string.h>
#include<inttypes.h>

#include <unistd.h>
#include<netdb.h>
#include<fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include"sbcp.h"

#define STDIN 0


#define VERSION 3
#define JOIN 2
#define SEND 4
#define FWD 3
#define ACK 7
#define NAK 5
#define ONLINE 8
#define OFFLINE 6
#define IDLE 9

#define USERNAME 2
#define MESSAGE 4
#define REASON 1
#define CLIENT_COUNT 3

#define MAX_USERNAME 16





void *get_in_addr(struct sockaddr *sa)
{
 if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
 }
     return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    char username[MAX_USERNAME];
    char buff[1024];
    memset(username, '\0', MAX_USERNAME);
    
   if(argc!=4){						// if user arguments are not equal to 4 then give an error
    printf("please use the format: ./client username server_ip server_port clients\n");
	return 1;
   }
   if(strlen(argv[1]) >= MAX_USERNAME){
    	printf("Name Too long! %d characters at most!\n", MAX_USERNAME-1); //15 character is max for username
    	exit(1);
   }
    memcpy(username, argv[1], strlen(argv[1])+1);
/*##################################Init  #####################################*/
    fd_set read_fds; 
    fd_set master;
    FD_ZERO(&master);  
   	FD_ZERO(&read_fds);
    FD_SET(STDIN,&master);
   
/*######################################Create Socket#########################*/
   struct addrinfo hints, *ai, *current;
   int sockfd; 
   int fdmax;
    char serverIP[INET6_ADDRSTRLEN];
   memset(&hints, 0, sizeof hints);  //clear hints;
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;
   if ( getaddrinfo(argv[2], argv[3], &hints, &ai) != 0) {
        perror("Get Information Failed\n");
        exit(2);
   }
   
/*############################################################################*/
 

  for(current = ai; current != NULL; current = current->ai_next) {
      sockfd = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
      if (sockfd < 0) {
          continue;
       }

      if (connect(sockfd, current->ai_addr, current->ai_addrlen) < 0) {
          close(sockfd);
          continue; 
      }
    break; 
  }

  if (current == NULL) {         
      perror("Connect Failed\n");    //check whether we bind successfully;
      exit(2); 
   }
	
  inet_ntop(current->ai_family, get_in_addr((struct sockaddr *)current->ai_addr),serverIP, sizeof serverIP);                                             // print the ip I am connecting
  printf("client: connecting to %s\n", serverIP);
  freeaddrinfo(ai);
/*############################################################################*/
  int bytes_sent;
  int bytes_recv; 
  uint16_t msg_vrsn = 3;
  uint8_t msg_type;
  char* newpack;
  char * attr_pack1 = attribute_to_string(USERNAME, username);
  char **attr_addr = (char **)malloc(sizeof(char*)*1);
  attr_addr[0] = attr_pack1;
  newpack = sbcp_to_string(VERSION,JOIN,1,attr_addr);
  struct sbcp_message *output = string_to_sbcp(newpack);
 /* printf("Version:%d\n",output->Vrsn);
  printf("Type:%d\n", output->Type);
  printf("Length:%d\n", output->Length);
  printf("Attribute1 Type %d\n", output->attr[0]->Type);
  printf("Attribute1 Length %d\n",output->attr[0]->Length);
  printf("Attribute1 payload %s\n", output->attr[0]->payload);*/
  
  if ((bytes_sent = send(sockfd, newpack, output->Length, 0)) == -1){   // meet error in send
        perror("send error in join");
        close(sockfd);
        exit(3);
  }
  printf("################JOIN HAS SENT#####################################\n");
  if ((bytes_recv = recv(sockfd, buff, sizeof(buff), 0)) <= 0) {
       // got error or connection closed by client
        perror("Receiving ACK/NAK Error");
        exit(4);
   }
  msg_vrsn = get_message_vrsn(buff);
  if(msg_vrsn != VERSION){
        perror("Version ERROR");
        exit(5);
   }
  msg_type = get_message_type(buff);
   struct sbcp_message *output1 =  string_to_sbcp(buff);
  if(msg_type == ACK){
      printf("Current Clients number are %s \n", output1->attr[0]->payload);
      printf("Current Clients with ACK:\n");
       printf("Local\n");
      int i;
      for(i = 1; i <output1->attr_number-1; i++){
          printf("%s\n", output1->attr[i]->payload);
      }
      printf("##################################################################\n");
  }
  else if(msg_type == NAK){
      printf("%s\n",output1->attr[0]->payload );
      exit(5);
  }
  else{
      perror("Unknown Message\n");
  }
  
  	FD_SET(sockfd,&master);
  	struct timeval t;
  	t.tv_sec = 10;		// Set idle timeout to 10 seconds
  	t.tv_usec = 0;
  	int idle = 0;
   fdmax = sockfd;
   int flag;
   int i; 
 while(1){
   flag = 0;
   read_fds = master;
		if( select(fdmax+1, &read_fds, NULL, NULL, &t) == -1){		// Select between client socket and STDIN file descriptors
			perror("client: select");
			exit(4);
		}
		if(!(FD_ISSET(STDIN,&read_fds)) && t.tv_sec==0 && idle==0){		// If client timeout occurs send IDLE message to server and reset the counter to 10
			idle = 1;
		 char *pack = sbcp_to_string(VERSION,IDLE,0,NULL);
			if( send(sockfd,pack,4,0) == -1){
				perror("client: send");
			}
			t.tv_sec = 10;
			printf("######################You are idle##################################\n");
		}
		for(i=0; i<=fdmax; i++){
			if(FD_ISSET(i,&read_fds)){
				if(i==sockfd){
				  int num_bytes;
					flag = 1;
					memset(buff,'\0',1024);
					num_bytes = recv(sockfd, buff, sizeof(buff),0);			// receive message from server
					if(num_bytes == -1){
						perror("client: recv");
					}
					else if(num_bytes == 0){
						printf("Connection closed by server\n");
						exit(7);									// if number of bytes received is zero, it means server has closed the connection
					}
					else{
						struct sbcp_message *output3 =  string_to_sbcp(buff);// process function to decode and taken actions on the received packet
						uint16_t vrsn = output3->Vrsn;
						if(vrsn != VERSION ){
						    perror("Unknown Message");
						    continue;
						}
						uint8_t type = output3->Type;
						if(type == FWD){
						    printf("%s : %s\n" , output3->attr[0]->payload,output3->attr[1]->payload);
						}
						else if(type == ONLINE){
						    printf("###############################%s ONLINE#############################\n", output3->attr[0]->payload);
						}
						else if(type == OFFLINE){
						    printf("###############################%s OFFLINE############################\n", output3->attr[0]->payload);
						}
						else if(type == IDLE){
						   printf("################################%s is IDLE############################\n", output3->attr[0]->payload);
						}
					}
				}
				if(i==STDIN){	// If user has entered something go inside and reset the IDLE counter to 10
					t.tv_sec = 10;
					idle = 0;
					char **attr_addr = (char **)malloc(sizeof(char*)*1);
					memset(buff, '\0', 1024);
					read(STDIN, buff, 1024);			// Get the user input
					buff[strlen(buff)-1] = '\0';       //remove \n;
					attr_addr[0] = attribute_to_string(MESSAGE,buff);		// pack the Message
					char *pack = sbcp_to_string(VERSION,SEND,1,attr_addr);		// get the final packet to be sent
					int len = unpacki16(pack+2);
					if(send(sockfd,pack,len,0) == -1){			// send the message to the server
						perror("client: send");
					}

				}
			}
		}
    		if (flag == 0){
		    	t.tv_sec = 10;	
    		}
}

 return 0;
 
}