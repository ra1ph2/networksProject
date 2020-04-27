// Akshit Khanna 2017A7PS0023P
#include "packet.h"

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
 
int main(int argc , char *argv[])
{
    srand(time(0));
    struct sockaddr_in si_me, si_other, si_client, si_server;
    int s, i, slen = sizeof(si_other) , recv_len;
    DATA_PKT recv_pkt, send_pkt;
    fd_set readfds;   
    int port_no;

    FILE *log;
    char relay[10];
    if(!strcmp(argv[1], "1"))
    {
        port_no = PORT_RELAY1;
        strcpy(relay, "RELAY1");
        log = fopen("relay1_log.txt", "w");
    }
    else
    {
        port_no = PORT_RELAY2;
        strcpy(relay, "RELAY2");
        log = fopen("relay2_log.txt", "w");
    }
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }

    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(port_no);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }

    memset((char *) &si_server, 0, sizeof(si_server));
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(PORT_SERVER);
    si_server.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("|Node Name |Event Type|Timestamp       |Pkt Type  |Seq. No.  |Source    |Dest      |\n");
    fprintf(log, "|Node Name |Event Type|Timestamp       |Pkt Type  |Seq. No.  |Source    |Dest      |\n");
         
    while(1)
    {
     
        if ((recv_len = recvfrom(s, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            die("recvfrom()");
        }

        char str[10];
        sprintf(str, "%d", recv_pkt.sq_no);
        if(recv_pkt.type == 0)
        {
            int drop = (rand()%100)+1;
            if(drop <= PROB)
            {
                printf("|%-10s|D         |%-16s|DATA      |%-10s|CLIENT    |%-10s|\n", relay, get_time(), str, relay);    
                fprintf(log, "|%-10s|D         |%-16s|DATA      |%-10s|CLIENT    |%-10s|\n", relay, get_time(), str, relay);    
                continue;
            }

            si_client = si_other; 
            int delay = rand()%3;
            usleep(delay);

            printf("|%-10s|R         |%-16s|DATA      |%-10s|CLIENT    |%-10s|\n", relay, get_time(), str, relay);    
            fprintf(log, "|%-10s|R         |%-16s|DATA      |%-10s|CLIENT    |%-10s|\n", relay, get_time(), str, relay);    
        
            if (sendto(s, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr*) &si_server, slen) == -1)
            {
                die("sendto()");
            }    
            printf("|%-10s|S         |%-16s|DATA      |%-10s|%-10s|SERVER    |\n", relay, get_time(), str, relay);    
            fprintf(log, "|%-10s|S         |%-16s|DATA      |%-10s|%-10s|SERVER    |\n", relay, get_time(), str, relay);    
        
        }
        else
        {
            printf("|%-10s|R         |%-16s|ACK       |%-10s|SERVER    |%-10s|\n", relay, get_time(), str, relay);    
            fprintf(log, "|%-10s|R         |%-16s|ACK       |%-10s|SERVER    |%-10s|\n", relay, get_time(), str, relay);    

            if (sendto(s, &recv_pkt, sizeof(recv_pkt), 0, (struct sockaddr*) &si_client, slen) == -1)
            {
                die("sendto()");
            }

            printf("|%-10s|S         |%-16s|ACK       |%-10s|%-10s|CLIENT    |\n", relay, get_time(), str, relay);    
            fprintf(log, "|%-10s|S         |%-16s|ACK       |%-10s|%-10s|CLIENT    |\n", relay, get_time(), str, relay);    
        
        }

        fflush(log);
    }
    fclose(log);
    close(s);
    return 0;
}
