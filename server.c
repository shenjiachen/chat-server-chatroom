
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
unsigned short int clientcount=0;
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
hints.ai_family = AF_UNSPEC;
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
                  if(msg_vrsn != VERSION){
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
                            printf("########################prepare ACK/NAK packet##########################\n");
                            char* newpack;
                            char* newpack1;
                            char reason[32] = "No more client";
                            unsigned int ACK_NAK = ACK;
                            if(clientcount > maxclient){
                                ACK_NAK = NAK;
                            }
                            char clientnumber[6];
                            char * number = itoa(clientcount, clientnumber, 10);
                            if(ACK_NAK == NAK){
                                char * attr_pack1 = attribute_to_string(REASON,reason);
                                char **attr_addr = (char **)malloc(sizeof(char*)*1);
                                attr_addr[0] = attr_pack1;
                                newpack = sbcp_to_string(VERSION,ACK_NAK,1,attr_addr);
                            }
                            else{
                               char **attr_addr = (char **)malloc(sizeof(char*)*(clientcount+1));
                               char **attr_addr1 = (char **)malloc(sizeof(char*)*1);
                               attr_addr[0] = attribute_to_string(CLIENT_COUNT,number);
                               attr_addr1[0] = attribute_to_string(USERNAME,username);
                               int i;
                               head = users;
                               for(i = 1; i <=clientcount; i++){
                                 if(head->next == NULL){
                                   attr_addr[i] = attribute_to_string(USERNAME,username);  
                                 }
                                 else{
                                   attr_addr[i] = attribute_to_string(USERNAME, head->next->username);
                                   head = head->next;
                                 }
                               }
                               newpack = sbcp_to_string(VERSION,ACK_NAK,clientcount+1,attr_addr);
                               newpack1 = sbcp_to_string(VERSION,ONLINE,1,attr_addr1);
                            }

                            struct sbcp_message *output = string_to_sbcp(newpack);
                            int len = unpacki16(newpack + 2);
                           /* printf("Version:%d\n",output->Vrsn);
                            printf("Type:%d\n", output->Type);
                            printf("Length:%d\n", output->Length);
                            printf("Attribute1 Type %d\n", output->attr[0]->Type);
                            printf("Attribute1 Length %d\n",output->attr[0]->Length);
                            printf("Attribute1 payload %s\n", output->attr[0]->payload);
                            printf("Attribute2 Type %d\n", output->attr[1]->Type);
                            printf("Attribute2 Length %d\n", output->attr[1]->Length);
                            printf("Attribute2 payload %s\n", output->attr[1]->payload);*/
                            client->sock_num = acceptedfd;
                            client->username = username;
                            client->next = NULL;
                            head->next = client;
                            if (send(acceptedfd, newpack,len , 0) == -1) {
                                    perror("Sending Failed");
                                    exit(6);
                            }
                            
                            printf("########################prepare ONLINE packet##########################\n");
                            len = unpacki16(newpack + 2);
                            
                            for(j = 0; j <= fdmax; j++) {
                             if (FD_ISSET(j, &master)) {
                                 if (j != listenfd && j != i) {// except the listener and ourselves
                                     if (send(j,newpack1,len, 0) == -1) {
                                          perror("Sending Failed");
                                          exit(6);
                                     }
                                }
                            }
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
                            printf("########################prepare OFFLINE packet##########################\n");
                            char *username;
                            struct user *head = users;
                            while(head->next != NULL){
                                  if(head->next->sock_num == i){
                                      username = head->next->username;
                                      break;
                                  }
                                  head = head->next;
                             }
                            char* newpack3;
                            char **attr_addr2 = (char **)malloc(sizeof(char*)*1);
                            attr_addr2[0] =  attribute_to_string(USERNAME,username);
                            newpack3 = sbcp_to_string(VERSION,OFFLINE,1,attr_addr2);
                            int len = unpacki16(newpack3 + 2);
                            for(j = 0; j <= fdmax; j++) {
                             if (FD_ISSET(j, &master)) {
                                 if (j != listenfd && j != i) {// except the listener and ourselves
                                     if (send(j,newpack3,len, 0) == -1) {
                                          perror("Sending Failed");
                                          exit(6);
                                     }
                                }
                            }
                          }
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
                        uint16_t msg_vrsn = get_message_vrsn(buff);
                        //printf("vrsn %d\n",msg_vrsn);
                        if(msg_vrsn != VERSION){
                           perror("Unkown Message");
                           continue;
                        }
                        uint8_t msg_type = get_message_type(buff);
                        //printf("type: %d\n",msg_type);
                        struct sbcp_message *output = string_to_sbcp(buff);
                        char *newpack;
                        int len;
                        if(msg_type == SEND){
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
                                      username = head->next->username;
                                      break;
                                  }
                                  head = head->next;
                             }
                            printf("########################prepare FWD packet##########################\n");
                            char *message = output->attr[0]->payload;
                            char * attr_pack1 = attribute_to_string(USERNAME, username);
                            char * attr_pack2 = attribute_to_string(MESSAGE, message);
                            attr_addr[0] = attr_pack1;
                            attr_addr[1] = attr_pack2;
                            newpack = sbcp_to_string(VERSION,FWD,2,attr_addr);
                            len = unpacki16(newpack+2);
                            struct sbcp_message *output = string_to_sbcp(newpack);
                           /* printf("Version:%d\n",output->Vrsn);
                            printf("Type:%d\n", output->Type);
                            printf("Length:%d\n", output->Length);
                            printf("Attribute1 Type %d\n", output->attr[0]->Type);
                            printf("Attribute1 Length %d\n",output->attr[0]->Length);
                            printf("Attribute2 payload %s\n", output->attr[0]->payload);
                            printf("Attribute2 Type %d\n", output->attr[1]->Type);
                            printf("Attribute2 Length %d\n", output->attr[1]->Length);
                            printf("Attribute2 payload %s\n", output->attr[1]->payload);*/
                         }
                     else if(msg_type == IDLE){
                        printf("########################prepare IDLE packet##########################\n");
                         char **attr_addr = (char **)malloc(sizeof(char*)*1);
                         char *username;
                            struct user *head = users;
                            while(head->next != NULL){
                                  if(head->next->sock_num == i){
                                      username = head->next->username;
                                      break;
                                  }
                            head = head->next;
                         }
                        // printf("User; %s\n",username);
                        char * attr_pack1 = attribute_to_string(USERNAME, username);
                        attr_addr[0] = attr_pack1;
                        newpack = sbcp_to_string(VERSION,IDLE,1,attr_addr);
                     }
                         for(j = 0; j <= fdmax; j++) {
                             int byte_sent;
                            if (FD_ISSET(j, &master)) {
                                if (j != listenfd && j != i) {// except the listener and ourselves
                                    if ((byte_sent=send(j,newpack,len, 0)) == -1) {
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