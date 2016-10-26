
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
      
 //sbcp message to string

 char *sbcp_to_string(uint16_t Vrsn, uint8_t Type, int attr_number, char **attr_addr){
    
    unsigned short int total_length = SBCP_Vrsn_Type_Size + SBCP_Length_Size;  
    int i;    // loop variable
    for(i = 0; i < attr_number; i++){
        total_length = total_length + unpacki16(attr_addr[i] + Attr_Type_Size); //get the total length of sbcp message
    }
    char *sbcp_string = (char *)malloc(sizeof(char) * total_length);   //allocate the memory for sbcp string;
	uint8_t vrsn = (Vrsn>>1);	
	sbcp_string[0] = vrsn;
	sbcp_string[1] = Type | ((Vrsn & 1) << 7);
    packi16(sbcp_string + SBCP_Vrsn_Type_Size, total_length);       //Length to string
    int attr_length;
    int current_length =SBCP_Vrsn_Type_Size + SBCP_Length_Size; 
    for(i = 0; i < attr_number; i++){
        attr_length = (unsigned int)unpacki16(attr_addr[i] + Attr_Type_Size);
        memset(sbcp_string + current_length, '\0', attr_length);
        memcpy(sbcp_string + current_length, attr_addr[i], attr_length );     //copy the sbcp attribute message
        current_length = current_length + attr_length;
    }
    return sbcp_string;
}    



//string to sbcp message
struct sbcp_message *string_to_sbcp(char *pack){
    struct sbcp_message *output = (struct sbcp_message *)malloc(sizeof(struct sbcp_message));
    uint16_t bits = unpacki16(pack);
    output->Type = (bits & 0x7F);
    output->Vrsn = (bits & 0xFF80) >> 7;
    output->Length = unpacki16(pack+SBCP_Vrsn_Type_Size);
    int attr_length;
    int attr_number = 0;
    int current_length = SBCP_Vrsn_Type_Size + SBCP_Length_Size;
    while(current_length < output->Length){
        attr_length = unpacki16(pack+current_length+Attr_Type_Size);
        current_length = current_length + attr_length;
        attr_number++;
    }
    output->attr_number = attr_number;
    output->attr = (struct sbcp_attr **)malloc(attr_number*sizeof(struct sbcp_attr *));
    int i;
    current_length = SBCP_Vrsn_Type_Size +SBCP_Length_Size;
    for(i = 0; i < attr_number; i++){
        attr_length = unpacki16(pack+current_length+Attr_Type_Size);
        output->attr[i] = string_to_attribute(pack+current_length);
        current_length = current_length + attr_length;
    }
    return output;
}

char * Clientnumber_to_char(unsigned short int clientcount){
    
}
int main(int argc, char *argv[])
{   char username[5];
    uint16_t msg_vrsn = (uint16_t)3;
    username[0] = 'a';
    username[1] = 'b';
    username[2] = 'c';
    username[3] = 'd';
    username[4] = '\0';
    char client[3];
    client[0] = '1';
    client[1] = '2';
    client[2] = '\0';
    
    char *newpack;
    char **attr_addr  =(char **)malloc(sizeof(char*)*2);
    char *attr_pack1 = attribute_to_string(CLIENT_COUNT,client);
    char *attr_pack2 = attribute_to_string(USERNAME, username);
    attr_addr[0] = attr_pack1;
    attr_addr[1] = attr_pack2;
    newpack = sbcp_to_string(msg_vrsn, ACK, 2, attr_addr);
    struct sbcp_message *output = string_to_sbcp(newpack);
    printf("Version:%d\n",output->Vrsn);
    printf("Type:%d\n", output->Type);
    printf("Length:%d\n", output->Length);
    printf("Attribute1 Type %d\n", output->attr[0]->Type);
    printf("Attribute1 Length %d\n",output->attr[0]->Length);
    printf("Attribute2 payload %s\n", output->attr[0]->payload);
    printf("Attribute2 Type %d\n", output->attr[1]->Type);
    printf("Attribute2 Length %d\n", output->attr[1]->Length);
    printf("Attribute2 payload %s\n", output->attr[1]->payload);
    
    return 0;
}