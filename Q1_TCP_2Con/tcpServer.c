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

    //set of socket descriptors  
    fd_set readfds;   
         
    //initialise all client_socket[] to 0 so not checked  
    for (i = 0; i < max_clients; i++)   
    {   
        client_socket[i] = 0;   
    }   
         
    //create a master socket  
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)   
    {   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }   
     
    //set master socket to allow multiple connections ,  
    //this is just a good habit, it will work without this  
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  
          sizeof(opt)) < 0 )   
    {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    }   
     
    //type of socket created  
    address.sin_family = AF_INET;   
    address.sin_addr.s_addr = INADDR_ANY;   
    address.sin_port = htons( PORT );   
         
    //bind the socket to localhost port 8888  
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)   
    {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }   
    printf("Listener on port %d \n", PORT);   
         
    //try to specify maximum of 3 pending connections for the master socket  
    if (listen(master_socket, 3) < 0)   
    {   
        perror("listen");   
        exit(EXIT_FAILURE);   
    }   
         
    //accept the incoming connection  
    addrlen = sizeof(address);   

    while(1)   
    {   
        //clear the socket set  
        FD_ZERO(&readfds);   
     
        //add master socket to set  
        FD_SET(master_socket, &readfds);   
        max_sd = master_socket;   
             
        //add child sockets to set  
        for ( i = 0 ; i < max_clients ; i++)   
        {   
            //socket descriptor  
            sd = client_socket[i];   
                 
            //if valid socket descriptor then add to read list  
            if(sd > 0)   
                FD_SET( sd , &readfds);   
                 
            //highest file descriptor number, need it for the select function  
            if(sd > max_sd)   
                max_sd = sd;   
        }   
     
        //wait for an activity on one of the sockets , timeout is NULL ,  
        //so wait indefinitely  
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);   
       
        if ((activity < 0) && (errno!=EINTR))   
        {   
            printf("select error");   
        }   
             
        //If something happened on the master socket ,  
        //then its an incoming connection  
        if (FD_ISSET(master_socket, &readfds))   
        {   
            if ((new_socket = accept(master_socket,  
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)   
            {   
                perror("accept");   
                exit(EXIT_FAILURE);   
            }   
            
            //add new socket to array of sockets  
            for (i = 0; i < max_clients; i++)   
            {   
                //if position is empty  
                if( client_socket[i] == 0 )   
                {   
                    client_socket[i] = new_socket;   
                    // printf("Adding to list of sockets as %d\n" , i);   
                         
                    break;   
                }   
            }   
        }   
             
        //else its some IO operation on some other socket 
        for (i = 0; i < max_clients; i++)   
        {   
            sd = client_socket[i];   
                 
            if (FD_ISSET( sd , &readfds))   
            {   
                //Check if it was for closing , and also read the  
                //incoming message  
                if ((valread = read( sd , &recv_pkt, sizeof(recv_pkt))) == 0)   
                {   
                    // //Somebody disconnected , get his details and print  
                    getpeername(sd , (struct sockaddr*)&address ,(socklen_t*)&addrlen);   
                    printf("Host disconnected , ip %s , port %d \n" ,  
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));   
                         
                    //Close the socket and mark as 0 in list for reuse  
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

                    // Drop packet randomly with 10 percent probabilty  
                    int drop = (rand()%100)+1;
                    if(drop <= PROB)
                    {
                        printf("DROP PKT: Seq No %d of size %d bytes from channel %d\n", recv_pkt.sq_no, recv_pkt.size,recv_pkt.channel);    
                        continue;
                    }
                    printf("RCVD PKT: Seq No %d of size %d bytes from channel %d\n", recv_pkt.sq_no, recv_pkt.size,recv_pkt.channel);
                    if(recv_pkt.sq_no == min_seq)
                    {
                        // min_seq = recv_pkt.sq_no + PKT_SIZE;
                        pkt_copy(&recv_pkt, &(buf_window[0]) ); // Structure Equivalence
                        for(int i = 0; i <= (max_seq-min_seq)/PKT_SIZE ; i++)
                        {
                            // printf("Package Data : %s\n",buf_window[i].data);
                            // printf("Packet %d received at server\n",buf_window[i].sq_no/PKT_SIZE);
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
