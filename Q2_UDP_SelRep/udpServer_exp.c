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
          
#define PORT 8003
#define BUFLEN 10
#define PROB 10  
#define BUFSIZE 4
#define DELAY 1
 
typedef struct packet{
    int size;
    int sq_no;
    int type; // 0 :- DATA  , 1 :- ACK
    int isLast; // 1 :- Last Packet
    char data[BUFLEN+1];
}DATA_PKT;

void pkt_copy(DATA_PKT *pkt1, DATA_PKT *pkt2)
{
    pkt2->size = pkt1->size;
    pkt2->sq_no = pkt1->sq_no;
    pkt2->type = pkt1->type;
    pkt2->isLast = pkt1->isLast;
    strcpy(pkt2->data, pkt1->data);
}

void die(char *s)
{
    perror(s);
    exit(1);
}
 
int main(int argc , char *argv[])
{
    struct sockaddr_in si_me, si_other;
    int s, i, slen = sizeof(si_other) , recv_len;
    DATA_PKT recv_pkt, send_pkt;
    DATA_PKT buf_window[BUFSIZE];   

    for(int k = 0 ; k < BUFSIZE ; k++)
    {
        buf_window[k].type = 0;
        buf_window[k].sq_no = -1;
    }
    
    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }

    FILE *fp = fopen("out.txt", "w");
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }
 
    int min_seq = 0;
    int isLast = 0;
    //keep listening for data
    while(1)
    {
        printf("Waiting for data...");
        fflush(stdout);

        int i = 0;
        // int filled_buf = BUFSIZE; 
        
        if ((recv_len = recvfrom(s, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            die("recvfrom()");
        }
        // if(i==0)
        //     min_seq = recv_pkt.sq_no;

        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        printf("Data: %s\n" , recv_pkt.data);    
        printf("Seq No: %d\n", recv_pkt.sq_no);

        pkt_copy(&recv_pkt, &(buf_window[ (recv_pkt.sq_no/ BUFLEN) % BUFSIZE]));
        i++;

        send_pkt.type = 1;
        send_pkt.sq_no = recv_pkt.sq_no;
        send_pkt.size = 0;
        send_pkt.isLast = recv_pkt.isLast;
        
        if (sendto(s, &send_pkt, sizeof(send_pkt), 0, (struct sockaddr*) &si_other, slen) == -1)
        {
            die("sendto()");
        }

        if(recv_pkt.isLast)
        {
            printf("REACHED\n");
            isLast = 1;
            // filled_buf = ((recv_pkt.sq_no - min_seq) / BUFLEN)+1;
            // printf("%d\n", filled_buf);
        }    
    
        if(recv_pkt.sq_no == min_seq)
        {
            int temp_seq = recv_pkt.sq_no;
            while(1)
            {        
                buf_window[(temp_seq/BUFLEN)%BUFSIZE].type = 2;
                printf("C\n");
                printf("%s\n", buf_window[(temp_seq/BUFLEN)%BUFSIZE].data);
                fwrite(buf_window[(temp_seq/BUFLEN)%BUFSIZE].data, 1, buf_window[(temp_seq/BUFLEN)%BUFSIZE].size, fp);

                if(buf_window[(temp_seq/BUFLEN)%BUFSIZE].sq_no + BUFLEN != buf_window[((temp_seq/BUFLEN)+1)%BUFSIZE].sq_no)
                    break;

                temp_seq += BUFLEN;    
            }
            min_seq = temp_seq+BUFLEN;
        }

        if(isLast)
        {
            int k;
            for(k =0 ; k < BUFSIZE ; k++)
            {
                if(buf_window[k].type != 2)
                    break;
            }
            printf("***** %d\n", k);
            if(k==BUFSIZE)
                break;
        }
    }
    fclose(fp);
    close(s);
    return 0;
}
