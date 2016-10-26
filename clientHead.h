//
//  clientHead.h
//  clientByMe
//
//  Created by Ke Peng on 9/19/15.
//  Copyright (c) 2015 Ke Peng. All rights reserved.

    /*Written by Fanchao Zhou*/
    /*The functions to pack and unpack messages at the client*/


//  Include some necessary header files, including the standard header files <string.h>,
//  and the header file containing the definitions of enumerators and structures.


#ifndef clientByMe_clientHead_h
#define clientByMe_clientHead_h

#include <string.h>
#include <stdbool.h>
#include <netinet/in.h>

#define SBCPVRSN 3
#define MAXMSGBUF 530
#define PRR_FORMATERR 1

enum MsgType
{
    JOIN = 2,
    FWD,
    SEND,
    NAK,
    OFFLINE,
    ACK,
    ONLINE
};

struct SBCPMsgHead
{
    unsigned short vrsn_and_type;
    unsigned short length;
};

enum AtbType
{
    REASON = 1,
    USERNAME,
    CLIENT_COUNT,
    MESSAGE
};

struct SBCPAtbHead
{
    unsigned short type;
    unsigned short length;
};


//Input: An array of characters received by recv() function.
//Output: The function returns a message type, or the negative value of the number of clients, or an error indicator.
//The information contained in the packet will be pasted into the input array, sbcp_packet.
//p_clients is used to return the number of clients
//Note: The elements in the original array used in recv() function should be set to zero first
int PacketReceived(char* sbcp_packet, int* p_clients)
{
    struct SBCPMsgHead msg_head;
    struct SBCPAtbHead atb_head;
    char temp_buf[ MAXMSGBUF ];
    
    *p_clients = -1;
    memcpy(&msg_head, sbcp_packet, sizeof(msg_head));                 //Get the message header
    msg_head.length = ntohs(msg_head.length);
	msg_head.vrsn_and_type = ntohs(msg_head.vrsn_and_type);
    memcpy(&atb_head, sbcp_packet+sizeof(msg_head), sizeof(atb_head));//Get the attribute header
    atb_head.length = ntohs(atb_head.length);
	atb_head.type = ntohs(atb_head.type);
    
    if(msg_head.vrsn_and_type>>7 != SBCPVRSN)                     //The version of SBCP is wrong
    {
        return PRR_FORMATERR;
    }
    else if(((msg_head.vrsn_and_type & 0x7F)!=ONLINE || atb_head.type!=USERNAME) &&
            ((msg_head.vrsn_and_type & 0x7F)!=OFFLINE || atb_head.type!=USERNAME) &&
            ((msg_head.vrsn_and_type & 0x7F)!=FWD || atb_head.type!=MESSAGE) &&
            ((msg_head.vrsn_and_type & 0x7F)!=NAK || atb_head.type!=REASON) &&
            ((msg_head.vrsn_and_type & 0x7F)!=ACK || atb_head.type!=CLIENT_COUNT))
    {//Either the message type or the attribute type is wrong
        return PRR_FORMATERR;
    }
    
    else
    {
        memset(temp_buf, '\0', MAXMSGBUF);
        switch(msg_head.vrsn_and_type & 0x7F)  //Check the message type
        {
            case ONLINE:
            case OFFLINE:
            case FWD:
            case NAK:
                memcpy(temp_buf,
                       sbcp_packet+sizeof(msg_head)+sizeof(atb_head),
                       atb_head.length-sizeof(atb_head));
                memcpy(sbcp_packet, temp_buf, MAXMSGBUF);//return the message in sbcp_packet
                break;
            case ACK:
                *p_clients = *(int *)(sbcp_packet+sizeof(msg_head)+sizeof(atb_head));//The number of clients
                *p_clients = ntohl(*p_clients);
                memcpy(temp_buf,
                       sbcp_packet+sizeof(msg_head)+sizeof(atb_head)*2+sizeof(int),
                       MAXMSGBUF-sizeof(msg_head)-sizeof(atb_head)*2-sizeof(int));//The list of names
                memcpy(sbcp_packet, temp_buf, MAXMSGBUF);
                break;
        }
        return msg_head.vrsn_and_type & 0x7F;  //return the message type
    }
}

//Input:A type of message to be sent, and the information carried by the packet
//Output:The information will be packed by the format of SBCP. And the packed packet
//       will be returned in the input array, msg. The length of the message is also returned
int PacketToSend(unsigned short msg_type, char* msg)
{
    unsigned short vrsn_and_type = 0;
    struct SBCPMsgHead msg_head;
    struct SBCPAtbHead atb_head;
    char sbcp_packet[ MAXMSGBUF ];          //The packet to be sent.
    char* cur_pos = sbcp_packet;
    char* temp_ptr;
    unsigned short msg_length;
    
    vrsn_and_type = SBCPVRSN;               //The current version of SBCP
    vrsn_and_type = vrsn_and_type<<7;       //The 9 high bits for version number
    vrsn_and_type = vrsn_and_type|msg_type; //The 7 low bits for message type
    msg_head.vrsn_and_type = vrsn_and_type;
    msg_head.vrsn_and_type = htons(msg_head.vrsn_and_type);
    
    memset(sbcp_packet, '\0', MAXMSGBUF);
    cur_pos = cur_pos+sizeof(struct SBCPMsgHead);   //reserve some space for message header
    
    if(msg_type==SEND || msg_type==JOIN)
    {
        temp_ptr = cur_pos;
        
        cur_pos = cur_pos+sizeof(struct SBCPAtbHead);   //reserve some space for the attribute header
        strcpy(cur_pos, msg);
        switch(msg_type)
        {
            case SEND:                     //The client is sending a message
                atb_head.type = MESSAGE;
                break;
            case JOIN:                     //The client is requesting to join
                atb_head.type = USERNAME;  //The client needs to send its name to the server
                break;
            default:
                break;
        }
        atb_head.type = htons(atb_head.type);
        atb_head.length = sizeof(atb_head)+strlen(msg)+1;
		msg_head.length = sizeof(msg_head)+atb_head.length;
		atb_head.length = htons(atb_head.length);
        memcpy(temp_ptr, &atb_head, sizeof(atb_head));   //Paste the header of the attribute
        
        msg_length = msg_head.length;
        msg_head.length = htons(msg_head.length);
        memcpy(sbcp_packet, &msg_head, sizeof(msg_head));//Paste the header of the message
        
        memcpy(msg, sbcp_packet, MAXMSGBUF);    //Return the packet to be sent by msg
        
        return msg_length;
    }
    else    //format error
    {
        return -1;
    }
    
}

#endif
