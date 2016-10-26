
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



#define Version 3
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

void *get_in_addr(struct sockaddr *sa)
{
 if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
 }
     return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


/* ########################### Main Function #################################*/



int main(int argc, char *argv[])
{
    
if(argc!=4){						// if user arguments are not equal to 4 then give an error
    printf("please use the format: ./server server_ip server_port max_clients\n");
	return 1;
}



/* ############################ Init  ########################################*/
unsigned short int maxclient;
unsigned short int clientcount;
unsigned short int BACKLOG;
fd_set master;       // master file descriptor list;
fd_set read_fds;     //  a temp file descriptor list; 


maxclient = atoi(argv[3]); //get the maximun clients from input;
BACKLOG = maxclient;
FD_ZERO(&master);    // clear the master and temp sets;
FD_ZERO(&read_fds);



/* ########################## Create Socket ##################################*/
struct addrinfo hints, *ai, *current;


memset(&hints, 0, sizeof hints);  //clear hints;
hints.ai_family = AF_INET;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;
if ( getaddrinfo(argv[1], argv[2], &hints, &ai) != 0) {
    perror("Get Information Failed\n");
    exit(1);
}




/* ############################## Bind #######################################*/

int listenfd;         // listen()ing file descriptor;
int acceptedfd;       // newly accept()ed file descriptor;


for(current = ai; current != NULL; current = current->ai_next) {
    listenfd = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
    if (listenfd < 0) {
        continue;
     }

    if (bind(listenfd, current->ai_addr, current->ai_addrlen) < 0) {
        close(listenfd);
        continue; 
    }
    break; 
}

if (current == NULL) {         
    perror("Bind Failed\n");    //check whether we bind successfully;
    exit(2); 
}
	

freeaddrinfo(ai);



/* ############################## Listen #################################### */
int fdmax = 0;           // highest file descriptor number;



if (listen(listenfd, BACKLOG) == -1) {
   perror("Listen Failed");
   exit(3);
}
printf("Get to the connection... ...\n");


FD_SET(listenfd, &master);   // add the listenfd to the master set;
fdmax = listenfd;        // keep track of the biggest file descriptor;

/* ############################## Select #################################### */
int i, j;            //loop variable;


struct sockaddr_storage clientaddr; // client address
char clientIP[INET6_ADDRSTRLEN];
socklen_t addrlen;
char buff[1024];     // buffer to store client data;
unsigned short int messagebytes;    // the length of the message;
struct user *users =  (struct user*)malloc(sizeof(struct user));
users->username = (char *)malloc(512*sizeof(char));
users->next = NULL;
while(1){
    read_fds = master; // copy master to read_fds
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
        perror("Select Failed");
        exit(4);
    }

 // run through the existing connections looking for data to read
    for(i = 0; i <= fdmax; i++){
        if(FD_ISSET(i, &read_fds)){

            if (i == listenfd) {
                  addrlen = sizeof(clientaddr);
                  acceptedfd = accept(listenfd, (struct sockaddr *)&clientaddr, &addrlen);
                  if (acceptedfd == -1) {
                      perror("Accepting Failed");
                      continue;
                  }
                  memset(buff, '\0', sizeof(buff));
                  if ((messagebytes = recv(acceptedfd, buff, sizeof(buff), 0)) <= 0) {
                     // got error or connection closed by client
                       perror("Receiving Message Error");
                       continue;
                   }
                  //check version
                  uint16_t msg_vrsn = get_message_vrsn(buff);
                  if(msg_vrsn != Version){
                      perror("Version ERROR");
                      continue;
                  }
                
                  //check message type
                  uint8_t msg_type = get_message_type(buff);
                  if(msg_type == JOIN){
                      struct user client;
                      struct sbcp_message *output =  string_to_sbcp(buff);
                      uint16_t attr_type = output->attr[0]->Type;
                      //check attribute type
                      if(attr_type == USERNAME){
                            clientcount++;
                            if(clientcount > maxclient){
                                 perror("No Client Anymore");
                                 continue;
                             }
                            char *username = output->attr[0]->payload;
                            // new a memory for new user
                            struct user *client= (struct user*)malloc(sizeof(struct user));
                            client->username = (char *)malloc(512*sizeof(char));
                            struct user *head = users;
                            int flag = 0;
                            while(head->next != NULL){
                                if(strcmp(head->next->username, username) == 0){
                                   perror("Client Name Duplicate");
                                   clientcount--;
                                   flag = 1;
                                   break;
                                }
                                head = head->next;
                             }
                            if(flag == 1){
                                close(acceptedfd);
                                 continue;     //there has duplicate name;
                            }
                            char *newpack;       // ACK packet
                            char **attr_addr  =(char **)malloc(sizeof(char*)*2);
                            char *clientnumber = (char*)malloc(sizeof(char)*2);
                            packi16(clientnumber, clientcount);
                            char *attr_pack1 = attribute_to_string(CLIENT_COUNT, clientnumber);
                            char *attr_pack2 = attribute_to_string(USERNAME, username);
                            attr_addr[0] = attr_pack1;
                            attr_addr[1] = attr_pack2;
                            newpack = sbcp_to_string(msg_vrsn, (uint8_t)ACK, 2, attr_addr);
                            client->sock_num = acceptedfd;
                            client->username = username;
                            client->next = NULL;
                            head->next = client;
                            if (send(acceptedfd, newpack,(unsigned int)output->Length , 0) == -1) {
                                perror("Sending ACK Failed");
                                exit(6);
                            }
                            FD_SET(acceptedfd, &master); // add to master set
                            if (acceptedfd > fdmax) { 
                                    fdmax = acceptedfd;     // keep track of the max
                            }
                            printf("selectserver: new connection on " "socket %d\n",acceptedfd);
                            //inet_ntop(clientaddr.ss_family,get_in_addr((struct sockaddr*)&clientaddr),clientIP, INET6_ADDRSTRLEN)
                      }
                  }
           }
           else {
                  if ((messagebytes = recv(i, buff, sizeof(buff), 0)) <= 0) {
                   // got error or connection closed by client
                      if (messagebytes == 0) {
                             printf("selectserver: socket %d hung up\n", i);
                      } 
                      else {
                           perror("Receiving Message Error");
                      }
                      close(i); // bye!
                      FD_CLR(i, &master); // remove from master set
                      struct user *head = users;
                      while(head->next != NULL){
                       if(head->next->sock_num == i){
                            head->next = head->next->next;
                            break;
                        }
                        head = head->next;
                      }
                      clientcount--;

                  }
                 else {  //message >0
                        uint16_t vrsn = get_message_vrsn(buff);
                        printf("vrsn %d\n",vrsn);
                        printf("%d\n",vrsn);
                        if(vrsn != Version){
                           perror("Unkown Message");
                           continue;
                        }
                        uint8_t type = get_message_type(buff);
                        struct sbcp_message *output;
                        char *newpack;
                        if(type == SEND){
                            output = string_to_sbcp(buff);
                            uint16_t attr_type = output->attr[0]->Type;
                            if(attr_type != MESSAGE){
                                perror("Unkown Message");
                                continue;
                            }
                            char **attr_addr = (char **)malloc(sizeof(char*)*2);
                            char *username;
                            struct user *head = users;
                            while(head->next != NULL){
                                  if(head->next->sock_num == i){
                                      username = users->next->username;
                                      break;
                                  }
                                  users = users->next;
                             }
                            char *message = output->attr[0]->payload;
                            char * attr_pack1 = attribute_to_string(USERNAME, username);
                            char * attr_pack2 = attribute_to_string(MESSAGE, message);
                            attr_addr[0] = attr_pack1;
                            attr_addr[1] = attr_pack2;
                            newpack = sbcp_to_string(vrsn,type,2,attr_addr);
                         }
                      
                         for(j = 0; j <= fdmax; j++) {
                            if (FD_ISSET(j, &master)) {
                                if (j != listenfd && j != i) { // except the listener and ourselves
                                    if (send(j, newpack,(unsigned int)output->Length , 0) == -1) {
                                          perror("Sending Failed");
                                          exit(6);
                                     }
                                }
                            }
                         }
                    }
              } 
        }
    } 
}

return 0;
}