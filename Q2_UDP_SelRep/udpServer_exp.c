// Akshit Khanna 2017A7PS0023P
#include "packet.h"

void pkt_copy(DATA_PKT *pkt1, DATA_PKT *pkt2)
{
    pkt2->size = pkt1->size;
    pkt2->sq_no = pkt1->sq_no;
    pkt2->type = pkt1->type;
    pkt2->isLast = pkt1->isLast;
    strcpy(pkt2->data, pkt1->data);
}

char * get_time()
{
    char *str = (char*) malloc(sizeof(char)*16);
    int rc;
    time_t cur;
    struct tm* timeptr;
    struct timeval tv;
    cur = time(NULL);
    timeptr = localtime(&cur);
    gettimeofday(&tv,NULL);
    rc = strftime(str, 16, "%H:%M:%S", timeptr);
    char ms[8];
    sprintf(ms,".%06ld",tv.tv_usec);
    strcat(str,ms);
    return str;
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
    
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }

    FILE *log = fopen("server_log.txt", "w");

    FILE *fp = fopen("out.txt", "w");
    
    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT_SERVER);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }
    printf("|Node Name |Event Type|Timestamp       |Pkt Type  |Seq. No.  |Source    |Dest      |\n");
    fprintf(log, "|Node Name |Event Type|Timestamp       |Pkt Type  |Seq. No.  |Source    |Dest      |\n");
    
    int min_seq = 0;
    int isLast = 0;
    while(1)
    {
        int i = 0;
        
        if ((recv_len = recvfrom(s, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            die("recvfrom()");
        }
        char str[10];
        sprintf(str, "%d", recv_pkt.sq_no);
        char relay[10];
        if(ntohs(si_other.sin_port) == PORT_RELAY1)
            strcpy(relay, "RELAY1");
        else
            strcpy(relay, "RELAY2");
        printf("|SERVER    |R         |%-16s|DATA      |%-10s|SERVER    |%-10s|\n", get_time(), str, relay);    
        fprintf(log, "|SERVER    |R         |%-16s|DATA      |%-10s|SERVER    |%-10s|\n", get_time(), str, relay);    

        pkt_copy(&recv_pkt, &(buf_window[ (recv_pkt.sq_no/ PKT_SIZE) % BUFSIZE]));
        i++;

        send_pkt.type = 1;
        send_pkt.sq_no = recv_pkt.sq_no;
        send_pkt.size = 0;
        send_pkt.isLast = recv_pkt.isLast;
        
        if (sendto(s, &send_pkt, sizeof(send_pkt), 0, (struct sockaddr*) &si_other, slen) == -1)
        {
            die("sendto()");
        }

        printf("|SERVER    |S         |%-16s|ACK       |%-10s|SERVER    |%-10s|\n", get_time(), str, relay);    
        fprintf(log, "|SERVER    |S         |%-16s|ACK       |%-10s|SERVER    |%-10s|\n", get_time(), str, relay);    

        if(recv_pkt.isLast)
        {
            isLast = 1;
        }    
    
        if(recv_pkt.sq_no == min_seq)
        {
            int temp_seq = recv_pkt.sq_no;
            while(1)
            {        
                buf_window[(temp_seq/PKT_SIZE)%BUFSIZE].type = 2;
                fwrite(buf_window[(temp_seq/PKT_SIZE)%BUFSIZE].data, 1, buf_window[(temp_seq/PKT_SIZE)%BUFSIZE].size, fp);

                if(buf_window[(temp_seq/PKT_SIZE)%BUFSIZE].sq_no + PKT_SIZE != buf_window[((temp_seq/PKT_SIZE)+1)%BUFSIZE].sq_no)
                    break;

                temp_seq += PKT_SIZE;    
            }
            min_seq = temp_seq+PKT_SIZE;
        }

        if(isLast)
        {
            int k;
            for(k =0 ; k < BUFSIZE ; k++)
            {
                if(buf_window[k].type != 2)
                    break;
            }
            if(k==BUFSIZE)
                break;
        }
    }
    fclose(fp);
    close(s);
    return 0;
}
