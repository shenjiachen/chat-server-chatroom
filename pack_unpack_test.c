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

void packi16 (char *buff, unsigned short int i)
{   //change the host order to network byte order (16bit)
	i = htons(i);
	memcpy(buff,&i,2);
}

unsigned short int unpacki16(char *buff)
{	//change  network byte order to the host order (16bit)
	unsigned short int i;
	memcpy(&i,buff,2);
	i = ntohs(i);
	return i;
}

int main(int argc, char *argv[])
{   char buff[2];
	unsigned short int number =1;
	printf("%d\n",number);
    packi16(buff, number);
	unsigned short int result = unpacki16(buff);
	printf("%d\n",result);
	
	return 0;
	
	
}