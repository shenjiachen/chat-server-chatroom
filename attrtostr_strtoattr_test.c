
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

#define SBCP_Vrsn_Type_Size 2
#define SBCP_Length_Size 2
#define Attr_Type_Size 2
#define Attr_Length_Size 2
#define MAX_Attr_Payload_Size 512



struct sbcp_message{
    uint16_t Vrsn;        //9 bits
    uint8_t  Type;        //7 bits
    uint16_t Length;      //2 bytes
    unsigned short int attr_number;
    struct sbcp_attr **attr;   //attribute array
};

struct sbcp_attr{
	uint16_t Type;       //2 bytes
	uint16_t Length;     //2 bytes
    char *payload;
};

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


//string to sbcp attribut
    struct sbcp_attr *string_to_attribute(char *pack){
         struct sbcp_attr *output = (struct sbcp_attr *)malloc(sizeof(struct sbcp_attr));
         output->Type = unpacki16(pack);
         output->Length = unpacki16(pack+Attr_Type_Size);
         output->payload = (char *)malloc(sizeof(char)*output->Length - Attr_Type_Size - Attr_Length_Size + 1);
         memset(output->payload, '\0', output->Length - Attr_Type_Size - Attr_Length_Size + 1);
         memcpy(output->payload,pack+Attr_Type_Size+Attr_Length_Size,(output->Length-Attr_Type_Size-Attr_Length_Size));
         return output;
     }
     
//sbcp attribute to string
      char *attribute_to_string(unsigned short int Type, unsigned char *payload ){
      unsigned short int total_length = Attr_Type_Size + Attr_Length_Size + strlen(payload);
      char *attr_string = (char *)malloc(sizeof(char) * total_length);
      packi16(attr_string, Type);
      packi16(attr_string + Attr_Type_Size, total_length);
      memset(attr_string+Attr_Type_Size + Attr_Length_Size, '\0', strlen(payload));
      memcpy(attr_string + Attr_Type_Size + Attr_Length_Size, payload, strlen(payload));
      return attr_string;
      }
      
      
int main(int argc, char *argv[])
{   
    char username[5];
    username[0] = 'a';
    username[1] = 'b';
    username[2] = 'c';
    username[3] = '\0';
    username[4] = '\0';
    printf("%d\n",strlen(username));
    char *attr_pack2 = attribute_to_string(USERNAME, username);
    struct sbcp_attr *output = string_to_attribute(attr_pack2);
    printf("Type: %d\n" ,output->Type);
    printf("Length; %d\n",output->Length);
    printf("Payload: %s\n", output->payload);
    return 0;
}