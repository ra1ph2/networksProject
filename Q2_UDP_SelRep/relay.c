/*  Simple udp server */
#include <stdio.h>  
#include <string.h>   //strlen  
#include <stdlib.h>  
#include <errno.h>  
#include <time.h>
#include <unistd.h>   //close  
#include <arpa/inet.h>    //close  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros  
          
#define PORT1 8001
#define PORT2 8002
#define PORT_SERVER 8003
#define BUFLEN 10
#define PROB 30  
#define BUFSIZE 4
#define DELAY 1
 
typedef struct packet{
    int size;
    int sq_no;
    int type; // 0 :- DATA  , 1 :- ACK
    int isLast; // 1 :- Last Packet
    char data[BUFLEN+1];
}DATA_PKT;

void die(char *s)
{
    perror(s);
    exit(1);
}
 
int main(int argc , char *argv[])
{
    srand(time(0));
    struct sockaddr_in si_me, si_other, si_client, si_server;
    int s, i, slen = sizeof(si_other) , recv_len;
    DATA_PKT recv_pkt, send_pkt;
    fd_set readfds;   
    int port_no;

    if(!strcmp(argv[1], "1"))
        port_no = PORT1;
    else
        port_no = PORT2;

    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
     
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(port_no);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }

    memset((char *) &si_server, 0, sizeof(si_server));
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(PORT_SERVER);
    si_server.sin_addr.s_addr = inet_addr("127.0.0.3");
     
    //keep listening for data
    while(1)
    {
        printf("Waiting for data...");
        fflush(stdout);
         
        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(s, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            die("recvfrom()");
        }

        if(recv_pkt.type == 0)
        {
            if(rand() % (100 / PROB) == 0)
            {
                continue;
            }

            si_client = si_other; 
            int delay = rand()%3;
            usleep(delay);

            printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
            printf("Data: %s\n" , recv_pkt.data);    
            printf("Seq No: %d\n", recv_pkt.sq_no);

            // pkt_copy(&recv_pkt, &send_pkt);
            if (sendto(s, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr*) &si_server, slen) == -1)
            {
                die("sendto()");
            }    
        }
        else
        {
            printf("Received Ack packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
            // printf("Data: %s\n" , recv_pkt.data);    
            printf("Seq No: %d\n", recv_pkt.sq_no);

            // pkt_copy(&recv_pkt, &send_pkt);
            if (sendto(s, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr*) &si_client, slen) == -1)
            {
                die("sendto()");
            }
        }
    }
    close(s);
    return 0;
}
