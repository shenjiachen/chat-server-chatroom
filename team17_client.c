//
//  main.c
//  TryClient
//
//  Created by Ke Peng on 9/13/15.
//  Copyright (c) 2015 Ke Peng. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "clientHead.h"


#define MAXDATASIZE 530
#define MAXNAMESIZE 15
#define STDIN 0  // file descriptor for standard input
#define max(a,b)    ((a) > (b) ? (a) : (b))
#define EXITNUM "@@exit\n"
#define SERVERDOWN "@@server"


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, const char * argv[]) {
    struct addrinfo hints, *res, *p;
    int status, sockfd = 0;
    char buf[MAXDATASIZE];
    char msg[MAXDATASIZE];
    char s[INET6_ADDRSTRLEN];
    int len, bytes_sent, bytes_rec;
    fd_set read_fds;  // read file descriptor  for select()
    fd_set write_fds;  // write file descriptor  for select
    int fdmax, i;        // maximum file descriptor number
    FILE* fp = stdin;
    int temp = 0;
    int p_client;
    int tempCount = 0;
    int sendtemp = 0;
    
    
    if (argc != 4) {
        fprintf(stderr,"usage: ./client username server_ip server_port \n");
        exit(1);
    }
    else if(strlen(argv[ 1 ]) >= MAXNAMESIZE){
    	printf("Name Too long! %d characters at most!\n", MAXNAMESIZE-1); //15 character is max for username
    	exit(1);
    }
    
    FD_ZERO(&read_fds);  // clean up read
    
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
    
    if ((status = getaddrinfo(argv[2], argv[3], &hints, &res)) != 0) {    // get address
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit (1);
    }
    
    // loop through all the results
    
    for (p = res; p != NULL; p = p->ai_next){
        if ((sockfd = socket(res->ai_family,
                             res->ai_socktype, res->ai_protocol)) == -1){
            perror("client: socket error");
            continue;
        }
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1){
            printf("%d\n", sockfd);
            perror("client: connect error");
            continue;
        }
        
        break;
        
    }
    
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit (1);
    }
    
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);                                             // print the ip I am connecting
    printf("client: connecting to %s\n", s);
    fflush(stdout);
    freeaddrinfo(res); // all done with this structure
    
    memset(msg, '\0', MAXDATASIZE);
    memset(buf, '\0', MAXDATASIZE);
    
    memcpy(msg, argv[1], strlen(argv[1])+1);
    
    
    if ((sendtemp = PacketToSend(JOIN, msg)) == -1){                // send a join package
        if ((bytes_sent =send(sockfd, EXITNUM, sendtemp, 0)) == -1){    // meet error in send join
            perror("send error in join");
        }
        perror("get join pack error");
        close(sockfd);
        exit (1);
        printf("Join send:%d\n", bytes_sent);
    }
    else{
        if ((bytes_sent = send(sockfd, msg, sendtemp, 0)) == -1){   // meet error in send
            perror("send error in join");
            close(sockfd);
            exit(1);
        }
    }
    
    
    if ((bytes_rec = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {  // meet error in receive
        perror("recv ack or nak error");
        exit(1);
    }
    else{
        printf("%d\n",bytes_rec);
        temp = PacketReceived(buf,&p_client);  
        printf("%d\n",temp);
        // receive ack or nak
        if (temp == NAK){   // nak condition
            fputs(buf,stdout);
            fflush(stdout);
            close(sockfd);
            exit(1);
        }else if (temp == ACK){
            printf("There are %d clients\nThey are:\n", p_client);
            fflush(stdout);
            fputs(buf,stdout);
            fflush(stdout);
            printf("\n");
            fflush(stdout);
        }else{
            perror("error in not recv ack or nak ");
            exit(1);
        }
    }
    
 
    
    fdmax = max(fileno(fp),sockfd);     // get the larger one between standard input and sockfd
 
    
    while (1){
        memset(msg, '\0', MAXDATASIZE);
        memset(buf, '\0', MAXDATASIZE);
        FD_SET(sockfd, &read_fds);  // read from sockfd
        FD_SET(fileno(fp), &read_fds);  // use standard input to write
        
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {   // select from standard input and socket
            perror("select");
            exit(1);
        }else{
        
            if(FD_ISSET(fileno(fp), &read_fds)){            // this is input condition
            
                fgets(msg, sizeof(msg), fp);
                fflush(stdin);                              // flush the memory immediately after input
                int temp2 = strcmp(msg,"@@exit\n");
                
                if (temp2 == 0){                        // meet the @@exit condition means client want to exit
                    if ((sendtemp = PacketToSend(SEND, msg)) == -1){
                        perror("get send pack exit error");
                        exit (1);
                    }else{
                        if ((bytes_sent = send(sockfd, msg, sendtemp, 0)) == -1){
                            perror("send exit error in SEND");
                            exit(1);
                        }
                        printf("%d\n", bytes_sent);
                    }
                    close(sockfd);
                    exit(1);
                }else{
                   if ((sendtemp = PacketToSend(SEND, msg)) == -1){
                        perror("get send pack error");
                        exit (1);
                    }
                    else{
                        if ((bytes_sent = send(sockfd, msg, sendtemp, 0)) == -1){
                            perror("send error in SEND");
                            exit(1);
                        }else{
                            continue;
                        }
                    }
                }
            
            }
        
            else
            {
                
                if ((bytes_rec = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
                    perror("recv");
                    exit(1);
                }else{
                
                    temp = PacketReceived(buf,&p_client);
                    if (temp == FWD){               // meet forward condition
                        fputs(buf,stdout);
                        fflush(stdout);
                    }else if (temp == ONLINE){      // meet other client is online condition
                        fputs("One client is online:",stdout);
                        fflush(stdout);
                        fputs(buf,stdout);
                        fflush(stdout);
                        fputs("\n",stdout);
                        fflush(stdout);
                    }else if (temp == OFFLINE){     // meet other client is offline condition
                    	if(strcmp(buf, SERVERDOWN)==0)      // check if the server is down
                    	{
                    		printf("The server is shut down!\n");
                    		fflush(stdout);
                    		close(sockfd);
                    		return 0;
                    	}
                    	else
                    	{                                   // meet condition that other client is offline
                        	fputs("One client is offline:",stdout);
                        	fflush(stdout);
                        	fputs(buf,stdout);
                        	fflush(stdout);
                        	fputs("\n",stdout);
                        	fflush(stdout);
                        }
                    }else{
                        tempCount++;
                        if (tempCount == 5){
                            exit(1);
                        }
                        continue;
                    }
                    FD_ZERO(&read_fds);
                }
            }
        }
    }
    
    
    
    
    close(sockfd);
    
    return 0;
    
}
