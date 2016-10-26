#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h> 
#include <sstream>
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>

#include<stdint.h>

#include<inttypes.h>

#include <unistd.h>
#include<netdb.h>
#include<fcntl.h>

#include <sys/socket.h>


using namespace std;

string _username;

enum Header_Type {JOIN, SEND, FWD, ERROR};
enum Attribute_Type {USERNAME, MESSAGE, REASON, CLIENTCOUNT, ATTRIBUTEERROR};

string CreatePacket(Header_Type type, string message)
{
	char header[4];
	bzero(&header, sizeof(header));
	header[0] = 1;
	header[1] = 128; 
	switch(type)
	{
		case JOIN:
			header[1]+= 2;
			break;
		case SEND:
			header[1]+= 4;		
			break;
		case FWD:
			header[1]+= 3;		
			break;
	}
	header[2] = (message.size()+4)>>8;
	header[3] = (message.size()+4)%256;	
	
	string header_string = "";
	for(int i = 0; i < 4 ; i++)
		 header_string += header[i];
	
	return header_string;
}

string CreateAttribute(Attribute_Type type, string message)
{
	char attribute[4];
	bzero(&attribute, sizeof(attribute));
	attribute[0]=0;
	switch(type)
	{
		case USERNAME:
			attribute[1] += 2;
			break;
		case MESSAGE:
			attribute[1] += 4;
			break;
		case REASON:
			attribute[1] += 1;
			break;
		case CLIENTCOUNT:
			attribute[1] += 3;
			break;
		default:
			attribute[1] += 10;
			break;
	}
	attribute[2] = (message.size()+4)>>8;
	attribute[3] = (message.size()+4)%256;

	string attribute_string = "";
	for(int i = 0; i < 4 ; i++)
		 attribute_string += attribute[i];
	
	return attribute_string;
}

Header_Type ReadHeadType(string message)
{	
	char type = message[1];
	type = type & 127;
	Header_Type HeaderType = ERROR;

    switch(type)
    {  
    	case 2:
    	    HeaderType = JOIN;
    	    break;
    	case 4:
    	    HeaderType = SEND;
    	    break;
    	case 3:
    	    HeaderType = FWD;
    	    break;
    }
	return HeaderType;
}

string RemoveHeader(string message)
{
	string message_string = "";
	int length = message[2]*256 + message[3];
	for(int i = 0; i < length; i++)
		message_string += message[i + 4];
	
	return message_string;
}

void err_sys(string x) 
{ 
    perror(x.c_str()); 
    exit(1); 
}

/////////////////////////////////////////////////////////////////////////////////////

//forward function
void Forward(string message)
{
	//remove the fwd header
	int length = message[2]*256 + message[3];
	message = RemoveHeader(message);
	
	//get the usename and text of received message
	string usernamerece = "";
	string messagerece = "";
	for(int i = 0; i < 2; i++)
	{
		int attributeType = message[1]; 
		int attributeLength = message[2]*256 + message[3]-4;
		if(attributeType == 2) 
		{
			usernamerece = "";
			for(int j = 0; j < attributeLength-4; j++)
				usernamerece += message[j + 4];
		}
		if(attributeType == 4)
		{
			messagerece = "";
			for(int j = 0; j < attributeLength-4; j++)
				messagerece += message[j + 4];			
		}
		
		//remove first part of attribute from message
		string messageadd = "";
		for(int j = attributeLength; j < message.size(); j++){
			messageadd += message[j];
		}
		message = messageadd;
	}
	
	//print to terminal the username followed by their message
	cout << usernamerece << ": " << messagerece << endl;
}

//multithreaded reading function
void Read(void* clientSocket)
{
	char buffer[1024]; 
	int sockfd = (int)clientSocket; 
	while(true)
	{
		bzero(&buffer, sizeof(buffer)); 
		int bytesRead = recv(sockfd,buffer,sizeof(buffer),0); 
		printf("%d\n",bytesRead);
		if(bytesRead == 0)
		{
			cout <<"Disconnected from Server ERROR" << endl;
			exit(0);
		}


        //get message from buffer
		string message = "";
		for(int i = 0; i < bytesRead; i++)
			message += buffer[i];
		
		//get header type
		Header_Type HeaderType = ReadHeadType(message);
        if(HeaderType == FWD){
        	Forward(message);
        }
	}
}

//multithreaded writing function
void Write(void* clientSocket)
{	
	char buffer[1024];
	int sockfd = (int)clientSocket;
	while(true)
	{
		bzero(&buffer, sizeof(buffer)); 
		string message = "";
		getline(cin, message);

		cout <<  "Local: " << message << endl;
		
		//add attribute and header
		message = CreateAttribute(MESSAGE, message) + message;
		message = CreatePacket(SEND, message) + message;
		
		//put the string into a buffer
		for(int i = 0; i < message.size(); i++)
			buffer[i] = message[i];
		
		//write data
		int bytesWritten = write(sockfd, buffer, message.size());
	}
}



///////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) 
{ 
	int sockfd, n; 
	const int MAXLINE = 1000;
	char recvline[MAXLINE + 1]; 
	struct sockaddr_in servaddr;
	int portnumber = -1;
		
	if (argc != 4)
	{
		cout << "Please input ./Client UserName ServerIpAdrress PortNumber" << endl;
		exit(1);
	}

	//create socket
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		err_sys("socket error"); 
	
	//parameters
	stringstream ss;
	ss << argv[3]; 
	ss >> portnumber;
	string ipaddr = argv[2];
	_username = argv[1];
	
	//connect
	bzero(&servaddr, sizeof(servaddr)); 
	
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(portnumber); 
	
	if (inet_pton(AF_INET, ipaddr.c_str(), &servaddr.sin_addr) <= 0) 
		err_sys("inet_pton error for " + ipaddr);
	
	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) 
		err_sys("connect error");
	
	//join
	string message = _username;
	message = CreateAttribute(USERNAME, message) + message;
	message = CreatePacket(JOIN, message) + message;
	
	char buffer[1024];
	bzero(&servaddr, sizeof(servaddr)); 
	
	for(int i = 0; i < message.size(); i++)
		buffer[i] = message[i];
	int bytesWritten = write(sockfd, buffer, message.size());
		
	//create threads for reading and writing
	pthread_t readThread;
	pthread_t writeThread;
	pthread_create(&readThread, NULL, Read, (void*)sockfd);
	pthread_create(&writeThread, NULL, Write, (void*)sockfd);
	while(true){};
	
	cout << "finished!" << endl;
}