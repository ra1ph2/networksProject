#include "packet.h"

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

int main(void)
{
    struct sockaddr_in si_relay1, si_relay2, si_other;
    int s, i, slen=sizeof(si_relay1);
    char buf[PKT_SIZE];
    char message[PKT_SIZE];
    DATA_PKT recv_pkt,send_pkt;
    DATA_PKT buf_window[BUFSIZE];
    fd_set readfds;
    struct timeval timeout;

    FILE* log = fopen("client_log.txt","w");

    FILE* fp = fopen("input.txt","ab");
    if(fp==NULL)
    {
        printf("\n Error in opening file");
        exit(0);
    }
    fseek(fp, 0, SEEK_END);
    int filesize = ftell(fp);
    fclose(fp);

    fp = fopen("input.txt", "rb");
 
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
 
    memset((char *) &si_relay1, 0, sizeof(si_relay1));
    si_relay1.sin_family = AF_INET;
    si_relay1.sin_port = htons(PORT_RELAY1);
    si_relay1.sin_addr.s_addr = inet_addr("127.0.0.1");

    memset((char *) &si_relay2, 0, sizeof(si_relay2));
    si_relay2.sin_family = AF_INET;
    si_relay2.sin_port = htons(PORT_RELAY2);
    si_relay2.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("|Node Name |Event Type|Timestamp       |Pkt Type  |Seq. No.  |Source    |Dest      |\n");
    fprintf(log, "|Node Name |Event Type|Timestamp       |Pkt Type  |Seq. No.  |Source    |Dest      |\n");
      
    while(1)
    {
        printf("************************\n");
        int i = 0;
        int min_seq;
        while(i < BUFSIZE && (send_pkt.size = fread(send_pkt.data, 1, PKT_SIZE, fp)))
        {
            send_pkt.data[send_pkt.size] = '\0';
            send_pkt.sq_no = ftell(fp)- send_pkt.size;
            if(i==0)
                min_seq = send_pkt.sq_no;
            send_pkt.type = 0;
            if(send_pkt.sq_no + PKT_SIZE < filesize)
                send_pkt.isLast = 0;
            else
                send_pkt.isLast = 1;
            
            pkt_copy(&send_pkt, &(buf_window[i]));
            if((send_pkt.sq_no / PKT_SIZE) % 2 )
            {
                // ODD CASE
                if (sendto(s, &send_pkt, sizeof(send_pkt) , 0 , (struct sockaddr *) &si_relay1, slen)==-1)
                {
                    die("sendto()");
                }
                char str[10];
                sprintf(str, "%d", send_pkt.sq_no);
                printf("|CLIENT    |S         |%-16s|DATA      |%-10s|CLIENT    |RELAY1    |\n", get_time(), str);    
                fprintf(log, "|CLIENT    |S         |%-16s|DATA      |%-10s|CLIENT    |RELAY1    |\n", get_time(), str);     
            }
            else
            {
                // EVEN CASE
                if (sendto(s, &send_pkt, sizeof(send_pkt), 0 , (struct sockaddr *) &si_relay2, slen)==-1)
                {
                    die("sendto()");
                }
                char str[10];
                sprintf(str, "%d", send_pkt.sq_no);
                printf("|CLIENT    |S         |%-16s|DATA      |%-10s|CLIENT    |RELAY2    |\n", get_time(), str);    
                fprintf(log, "|CLIENT    |S         |%-16s|DATA      |%-10s|CLIENT    |RELAY2    |\n", get_time(), str);    
            }
            i++;
        }
   
        int filled_buf = i;
        int seq = 0;
        i = 0;
        int last_seq = 10000;
        while(1)
        { 
            FD_ZERO(&readfds);
            FD_SET(s, &readfds);

            timeout.tv_sec = TIMEOUT;
            timeout.tv_usec = 0;

            // int k;
            // for(k =0 ; k < BUFSIZE ; k++)
            // {
            //     if(buf_window[k].type != 2)
            //         break;
            // }
            // if(k==BUFSIZE)
            //     break;

            int pkt_ack = select(s+1, &readfds, NULL, NULL, &timeout); 
            if(pkt_ack <= 0)
            {
                // Connection Timeout
                int tp = 0;
                while(tp < BUFSIZE)
                {
                    if(buf_window[((seq/PKT_SIZE)+tp)%BUFSIZE].type != 2)
                    {
                        pkt_copy(&(buf_window[((seq/PKT_SIZE)+tp)%BUFSIZE]), &send_pkt);
                        printf("|CLIENT    |TO        |%-16s|NA        |NA        |NA        |NA        |\n", get_time());    
                        fprintf(log, "|CLIENT    |TO        |%-16s|NA        |NA        |NA        |NA        |\n", get_time());    
                        
                        if((send_pkt.sq_no / PKT_SIZE) % 2 )
                        {
                            // ODD CASE
                            if (sendto(s, &send_pkt, sizeof(send_pkt), 0 , (struct sockaddr *) &si_relay1, slen)==-1)
                            {
                                die("sendto()");
                            }
                            char str[10];
                            sprintf(str, "%d", send_pkt.sq_no);
                            printf("|CLIENT    |RE        |%-16s|DATA      |%-10s|CLIENT    |RELAY1    |\n", get_time(), str);                    
                            fprintf(log, "|CLIENT    |RE        |%-16s|DATA      |%-10s|CLIENT    |RELAY1    |\n", get_time(), str);                    
                        
                        }
                        else
                        {
                            // EVEN CASE
                            if (sendto(s, &send_pkt, sizeof(send_pkt), 0 , (struct sockaddr *) &si_relay2, slen)==-1)
                            {
                                die("sendto()");
                            }
                            char str[10];
                            sprintf(str, "%d", send_pkt.sq_no);
                            printf("|CLIENT    |RE        |%-16s|DATA      |%-10s|CLIENT    |RELAY2    |\n", get_time(), str);                
                            fprintf(log, "|CLIENT    |RE        |%-16s|DATA      |%-10s|CLIENT    |RELAY2    |\n", get_time(), str);                
                        
                        }
                    }
                    tp++;
                }
            }
            else
            {
                // TODO : DIFFRENTIATE BETWEEN THE ACKS FROM BOTH THE RELAYS
                if (recvfrom(s, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr *) &si_other, &slen) == -1)
                {
                    die("recvfrom()");
                }

                buf_window[(recv_pkt.sq_no/PKT_SIZE)%BUFSIZE].type = 2;
                
                char str[10];
                sprintf(str, "%d", recv_pkt.sq_no);
                char relay[10];
                if(ntohs(si_other.sin_port) == PORT_RELAY1)
                    strcpy(relay, "RELAY1");
                else
                    strcpy(relay, "RELAY2");
                printf("|CLIENT    |R         |%-16s|ACK       |%-10s|CLIENT    |%-10s|\n", get_time(), str, relay);    
                fprintf(log, "|CLIENT    |R         |%-16s|ACK       |%-10s|CLIENT    |%-10s|\n", get_time(), str, relay);    
            
                // if(recv_pkt.sq_no == last_seq)
                //     break;

                if(recv_pkt.sq_no == seq)
                {
                    int tp = 0;
                    while(tp++ < BUFSIZE)
                    {
                        if(buf_window[((seq / PKT_SIZE)+(tp-1))%BUFSIZE].type != 2)
                        {
                            seq = seq + (tp-1)*PKT_SIZE;
                            break;
                        }
                        
                        if(tp == BUFSIZE)
                        {
                            seq = seq + tp*PKT_SIZE;   
                        }    
                    }
                    if(seq > last_seq)
                        break;
                    // printf("NEW SEQ ***** %d\n", seq);
                    if((send_pkt.size = fread(send_pkt.data, 1, PKT_SIZE, fp)))
                    {
                        send_pkt.data[send_pkt.size] = '\0';
                        send_pkt.sq_no = ftell(fp)- send_pkt.size;
                        send_pkt.type = 0;
                        if(send_pkt.sq_no + PKT_SIZE < filesize)
                        {
                            send_pkt.isLast = 0;
                        }
                        else
                        {
                            last_seq = send_pkt.sq_no;
                            send_pkt.isLast = 1;
                        }
                    }
                    else
                        continue;

                    tp = BUFSIZE - (send_pkt.sq_no - seq)/PKT_SIZE;
                    while(tp--)
                    {
                        if((send_pkt.sq_no / PKT_SIZE) % 2 )
                        {
                            // ODD CASE
                            if (sendto(s, &send_pkt, sizeof(send_pkt) , 0 , (struct sockaddr *) &si_relay1, slen)==-1)
                            {
                                die("sendto()");
                            }
                            char str[10];
                            sprintf(str, "%d", send_pkt.sq_no);
                            printf("|CLIENT    |S         |%-16s|DATA      |%-10s|CLIENT    |RELAY1    |\n", get_time(), str);                        
                            fprintf(log, "|CLIENT    |S         |%-16s|DATA      |%-10s|CLIENT    |RELAY1    |\n", get_time(), str);                        
                        
                        }
                        else
                        {
                            // EVEN CASE
                            if (sendto(s, &send_pkt, sizeof(send_pkt), 0 , (struct sockaddr *) &si_relay2, slen)==-1)
                            {
                                die("sendto()");
                            }
                            char str[10];
                            sprintf(str, "%d", send_pkt.sq_no);
                            printf("|CLIENT    |S         |%-16s|DATA      |%-10s|CLIENT    |RELAY2    |\n", get_time(), str);                    
                            fprintf(log, "|CLIENT    |S         |%-16s|DATA      |%-10s|CLIENT    |RELAY2    |\n", get_time(), str);                    
                        
                        }
                        pkt_copy(&send_pkt, &(buf_window[(send_pkt.sq_no/PKT_SIZE)%BUFSIZE]));
                        if(tp == 0)
                            break;
                        if((send_pkt.size = fread(send_pkt.data, 1, PKT_SIZE, fp)))
                        {
                            send_pkt.data[send_pkt.size] = '\0';
                            send_pkt.sq_no = ftell(fp)- send_pkt.size;
                            send_pkt.type = 0;
                            if(send_pkt.sq_no + PKT_SIZE < filesize)
                            {
                                send_pkt.isLast = 0;
                            }
                            else
                            {
                                last_seq = send_pkt.sq_no;
                                send_pkt.isLast = 1;
                            }
                        }
                        else
                            break;
                    }
                }
            }
        }

        if(feof(fp))
            break;
    }
    fclose(log);
    close(s);
    return 0;
}
