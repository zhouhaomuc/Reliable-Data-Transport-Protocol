/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
 * VERSION: 0.2
 * AUTHOR: Kai Shen (kshen@cs.rochester.edu)
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  1 byte  ->|<-             the rest            ->|
 *       | payload size |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_sender.h"
packet* packets;
int total=0;
int window=10;
//int send=0;
//int bound=window;
int goback=0;
//int loop=0;

unsigned short int checksum_rec(char *addr, short head, short seq, int count)
{
  register int sum = 0;

  // Main summing loop
  while(count > 1)
  {
    sum = sum + *((unsigned short int *) addr);
    addr+=2;
    count = count - 2;
  }

  // Add left-over byte, if any
  if (count > 0)
    sum = sum + *((char *) addr);
  sum += head;
  sum += seq;

  // Fold 32-bit sum to 16 bits
  while (sum>>16)
    sum = (sum & 0xFFFF) + (sum >> 16);

  return(~sum);
}
/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    packets=(struct packet*)malloc(sizeof(struct packet)*100000);
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    /* 1-byte header indicating the size of the payload */
    //printf(">>>>>SEND\n");
    int header_size = 4;

    /* maximum payload size */
    int maxpayload_size = RDT_PKTSIZE - header_size;

    /* split the message if it is too big */

    /* reuse the same packet data structure */
    packet pkt;

    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;
    //int seqnum=0;

    while (msg->size-cursor > maxpayload_size) {
    	/* fill in the packet */
    	pkt.data[0] = maxpayload_size;
      pkt.data[1] = total%50;
      //printf("%d\n",pkt.data[1]);
    	memcpy(pkt.data+header_size, msg->data+cursor, maxpayload_size);
      unsigned short int check= checksum_rec(pkt.data+4,pkt.data[0],pkt.data[1],60);
      memcpy(pkt.data+2, &check, 2);
    	/* send it out through the lower layer */
      packets[total]=pkt;
    	//Sender_ToLowerLayer(&packets[total]);
      //printf("%d\n",total);
      total++;

    	/* move the cursor */
    	cursor += maxpayload_size;
    }

    /* send out the last packet */
    if (msg->size > cursor) {
    	/* fill in the packet */
    	pkt.data[0] = msg->size-cursor;
      pkt.data[1] = total%50;
      //printf("SEND%d\n",pkt.data[1]);

    	memcpy(pkt.data+header_size, msg->data+cursor, pkt.data[0]);
      unsigned short int check= checksum_rec(pkt.data+4,pkt.data[0],pkt.data[1],60);
      memcpy(pkt.data+2, &check, 2);
      packets[total]=pkt;
      //printf("SEND%d\n",total);
      //Sender_ToLowerLayer(&packets[total]);
      total++;
    	/* send it out through the lower layer */
    	//Sender_ToLowerLayer(&pkt);
    }

    //************* send ******************
    

    int i=0;


    if(!Sender_isTimerSet()){
      Sender_StartTimer(0.3);
    }

    int bound=0;
    if(total<goback+window){
      bound=total;
    }else{
      bound=goback+window;
    }

    for(i=goback;i<bound;i++){
      printf("send %d \n", i);
      Sender_ToLowerLayer(&packets[i]);
    }
      
      return;
    //}
    //}
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{

  /*int acknum=0;
  acknum = pkt->data[0];*/
  
  //printf("^^^^TRUEACK %d \n",pkt->data[1]);
  unsigned short int check_ack= checksum_rec(pkt->data,(short)0,(short)0,2);
  unsigned short int check_sender = *(unsigned short int *) (pkt->data + 2);
  if(check_ack == check_sender){
    int ack=pkt->data[1];
    printf("know ack %d\n",ack);
    int comp=ack-(goback%50);
    printf("total %d\n",total);
    printf("comp %d\n",comp);
    printf("goback%d\n",goback);
    if(comp==0 && goback==total){
      printf("=0stoptimer\n");
      Sender_StopTimer();
      return;

    }
    if(comp>0 && comp<window && comp<=(total - goback) ){
      if(comp==(total - goback )){
        printf(">0stoptimer\n");
        Sender_StopTimer();
        return;
      }
      goback+=comp;
      printf(">0settimer\n");
      Sender_StartTimer(0.3);
    }
    if(comp<0 && (comp+50)<window && (comp+50)<=(total - goback)){
      if((comp+50)==(total - goback)){
        printf("<0stoptimer\n");
        Sender_StopTimer();
        return;
      }
      goback+=(comp+50);
      printf("<0settimer\n");
      Sender_StartTimer(0.3);
    }
  }
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
  /*printf("***********TIMEOUT\n");
  send=goback+1;
  printf("RESET SEND:%d\n",send);
  printf("BOUND %d\n",bound);
  printf("TOTAL%d\n",total);
  Sender_StopTimer();
  return;*/
  int i=0;
  int bound=0;
  if(total<goback+window){
    bound=total;
  }else{
    bound=goback+window;
  }

  for(i=goback;i<bound;i++){
    Sender_ToLowerLayer(&packets[i]);
  }
  printf("timeout timer\n");
  Sender_StartTimer(0.3);

}


