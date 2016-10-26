#define SBCP_Vrsn_Type_Size 2
#define SBCP_Length_Size 2
#define Attr_Type_Size 2
#define Attr_Length_Size 2
#define MAX_Attr_Payload_Size 512


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


//define protocal
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

//define user
struct user{
     int sock_num;
     char *username;
     struct user *next;
};

char *itoa (int value, char *result, int base)
{
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
  
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


//get message vrsn
uint16_t get_message_vrsn(char *pack){
    uint16_t bits = unpacki16(pack);
    uint16_t Vrsn = (bits & 0xFF80) >> 7;
    return Vrsn;
}
//get message type
uint8_t get_message_type(char *pack){
    uint8_t Type =  0x7F & pack[1];
    return Type;
}








