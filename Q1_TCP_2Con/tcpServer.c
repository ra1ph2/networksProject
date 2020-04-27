// Akshit Khanna 2017A7PS0023P
#include "packet.h"

void pkt_copy(DATA_PKT *pkt1, DATA_PKT *pkt2)
{
    pkt2->size = pkt1->size;
    pkt2->sq_no = pkt1->sq_no;
    pkt2->type = pkt1->type;
    pkt2->isLast = pkt1->isLast;
    pkt2->channel = pkt1->channel;
    strcpy(pkt2->data, pkt1->data);
}

int main(int argc , char *argv[])   
{   
    srand(time(0));
    int opt = 1;   
    int master_socket , addrlen , new_socket , client_socket[MAX_CON] ,max_clients = MAX_CON , activity, i , valread , sd;   
    int max_sd;   
    struct sockaddr_in address;   
    
    int min_seq = 0, max_seq = 0;
    DATA_PKT buf_window[BUFSIZE];
    DATA_PKT recv_pkt, send_pkt;

    FILE *fp_out = fopen("output_tcp.txt", "w");     

    fd_set readfds;   
  
    for (i = 0; i < max_clients; i++)   
    {   
        client_socket[i] = 0;   
    }   
  
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)   
    {   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }   
     
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  
          sizeof(opt)) < 0 )   
    {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    }   
     
    address.sin_family = AF_INET;   
    address.sin_addr.s_addr = INADDR_ANY;   
    address.sin_port = htons( PORT );   
         
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)   
    {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }   
    printf("Listener on port %d \n", PORT);   
   
    if (listen(master_socket, 3) < 0)   
    {   
        perror("listen");   
        exit(EXIT_FAILURE);   
    }   
         
    addrlen = sizeof(address);   

    while(1)   
    {   
        FD_ZERO(&readfds);   
     
        FD_SET(master_socket, &readfds);   
        max_sd = master_socket;   
             
        for ( i = 0 ; i < max_clients ; i++)   
        {   
            sd = client_socket[i];   
                 
            if(sd > 0)   
                FD_SET( sd , &readfds);   
                 
            if(sd > max_sd)   
                max_sd = sd;   
        }   
     
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);   
       
        if ((activity < 0) && (errno!=EINTR))   
        {   
            printf("select error");   
        }   
             
        if (FD_ISSET(master_socket, &readfds))   
        {   
            if ((new_socket = accept(master_socket,  
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)   
            {   
                perror("accept");   
                exit(EXIT_FAILURE);   
            }   
            
            for (i = 0; i < max_clients; i++)   
            {   
                if( client_socket[i] == 0 )   
                {   
                    client_socket[i] = new_socket;   
                         
                    break;   
                }   
            }   
        }   
             
        for (i = 0; i < max_clients; i++)   
        {   
            sd = client_socket[i];   
                 
            if (FD_ISSET( sd , &readfds))   
            {   
                if ((valread = read( sd , &recv_pkt, sizeof(recv_pkt))) == 0)   
                {   
                    getpeername(sd , (struct sockaddr*)&address ,(socklen_t*)&addrlen);   
                    printf("Host disconnected , ip %s , port %d \n" ,  
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));   
                         
                    close( sd );   
                    client_socket[i] = 0;   

                    int flag;
                    for(flag = 0 ; flag < 2 ; flag++)
                    {
                        if(client_socket[flag] != 0)
                            break;
                    }
                    if(flag == 2)
                    {
                        fclose(fp_out);
                        exit(0);
                    }
                }   
                     
                else 
                {   

                    int drop = (rand()%100)+1;
                    if(drop <= PROB)
                    {
                        printf("DROP PKT: Seq No %d of size %d bytes from channel %d\n", recv_pkt.sq_no, recv_pkt.size,recv_pkt.channel);    
                        continue;
                    }
                    printf("RCVD PKT: Seq No %d of size %d bytes from channel %d\n", recv_pkt.sq_no, recv_pkt.size,recv_pkt.channel);
                    if(recv_pkt.sq_no == min_seq)
                    {
                        pkt_copy(&recv_pkt, &(buf_window[0]) ); // Structure Equivalence
                        for(int i = 0; i <= (max_seq-min_seq)/PKT_SIZE ; i++)
                        {
                            fprintf(fp_out, "%s", buf_window[i].data);
                        }
                        min_seq = max_seq + PKT_SIZE;
                        max_seq = min_seq;

                        send_pkt.type = 1;
                        send_pkt.sq_no = recv_pkt.sq_no;
                        send_pkt.size = 0;
                        send_pkt.channel = recv_pkt.channel;
                        send(sd, &send_pkt, sizeof(send_pkt), 0);
                        printf("SENT ACK: for PKT with Seq No %d from channel %d\n", send_pkt.sq_no, send_pkt.channel);
                    }   
                    else
                    {
                        if((recv_pkt.sq_no - min_seq)/PKT_SIZE < BUFSIZE )
                        {
                            max_seq = recv_pkt.sq_no;
                            pkt_copy(&recv_pkt, &(buf_window[(recv_pkt.sq_no - min_seq)/PKT_SIZE]) ); // Structure Equivalence

                            send_pkt.type = 1;
                            send_pkt.sq_no = recv_pkt.sq_no;
                            send_pkt.size = 0;
                            send_pkt.channel = recv_pkt.channel;
                            send(sd, &send_pkt, sizeof(send_pkt), 0);
                            printf("SENT ACK: for PKT with Seq No %d from channel %d\n", send_pkt.sq_no, send_pkt.channel);
                        }
                        else
                        {
                            continue;
                        }
                    }  
                }   
            }   
        }   
    }   
         
    return 0;   
}   
