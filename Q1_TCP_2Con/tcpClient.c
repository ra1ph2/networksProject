#include <stdio.h>  
#include <string.h>   //strlen  
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>   //close  
#include <arpa/inet.h>    //close  
#include <sys/types.h>  
#include <sys/wait.h>
#include <sys/stat.h>    
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros  
     
#define MAX_CON 2     
#define PORT 8888
#define BUFLEN 10
#define PROB 10  
#define BUFSIZE 4
#define SEM_NAME "/sem"
#define TIMEOUT 2

typedef struct packet{
    int size;
    int sq_no;
    int type; // 0 :- DATA  , 1 :- ACK
    int isLast; // 1 :- Last Packet
    int channel;
    char data[BUFLEN+1];
}DATA_PKT;


int main()
{
    
    int client_socket;
	struct sockaddr_in serverAddress;		//Note that this is the *server* address.
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	// printf("Address assigned.\n");
    int connection;
	int bytes_sent;
	int bytes_recd;

    int client_socket1 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(client_socket1 < 0){
        printf("An error occurred while creating the client socket\n");
        exit(0);
    }
    int client_socket2 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(client_socket2 < 0){
        printf("An error occurred while creating the client socket\n");
        exit(0);
    }

    FILE* fp = fopen("input.txt","ab");
    if(fp==NULL)
    {
        printf("\n Error in opening file");
        exit(0);
    }
    fseek(fp, 0, SEEK_END);
    int filesize = ftell(fp);

    fp = fopen("input.txt", "rb");
    setvbuf(fp, NULL, _IONBF, 0);
    pid_t pid;
    struct timeval timeout;
    fd_set readfds;
    sem_t *sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP), 1);

    if((pid = fork()) == 0 )
    {
        
        printf("The client socket %d (Channel 1) was successfully created.\n", client_socket1);       
        connection = connect(client_socket1, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
        if(connection < 0){
            printf("An error occurred while connecting client to server.\n");
            exit(0);
        }
        printf("Connection 1 established successfully.\n");
    
        sem_t *sema = sem_open(SEM_NAME, O_RDWR);
        DATA_PKT recv_pkt,send_pkt;
        while(1)
        {
            // int temp = ftell(fp);
            sem_wait(sema);
            send_pkt.size = fread(send_pkt.data ,1, BUFLEN, fp);
            send_pkt.sq_no = ftell(fp)-send_pkt.size;
            sem_post(sema);
            // if(send_pkt.sq_no - temp != 0)
            //     send_pkt.sq_no -= BUFLEN; 
            if(send_pkt.size == 0)
                break;
            // printf("AAAAAAAAAAA\n");
            send_pkt.data[send_pkt.size] = '\0';
            send_pkt.channel = 1;
            // printf("A:- Debug Data : %s\n", send_pkt.data);
            // printf("A:- SEQ NO : %d\n", send_pkt.sq_no);
            if(send_pkt.sq_no + BUFLEN < filesize)
            {
                send_pkt.isLast = 0;
            }
            else
            {
                // printf("XXXXXXXX\n");
                send_pkt.isLast = 1;
            }
            send_pkt.type = 0;

            // FD_ZERO(&readfds);
            // FD_SET(client_socket1, &readfds);

            // timeout.tv_sec = 2;
            // timeout.tv_usec = 0;

            bytes_sent = send(client_socket1, &send_pkt, sizeof(send_pkt), 0);
            if(bytes_sent != sizeof(send_pkt)){
                printf("An error occurred while sending the message to the server.\n");
                exit(0);
            }
            printf("SENT PKT: Seq No %d of size %d bytes from channel %d\n", send_pkt.sq_no, send_pkt.size, send_pkt.channel);
            while(1)
            {
                
                FD_ZERO(&readfds);
                FD_SET(client_socket1, &readfds);

                timeout.tv_sec = TIMEOUT;
                timeout.tv_usec = 0;

                int pkt_ack = select(client_socket1+1, &readfds, NULL, NULL, &timeout);
                if(pkt_ack <= 0)
                {
                    // Connection Timeout
                    // printf("Connection Timeout for Packet %d.", send_pkt.sq_no/BUFLEN);
                    bytes_sent = send(client_socket1, &send_pkt, sizeof(send_pkt), 0);
                    if(bytes_sent != sizeof(send_pkt)){
                        printf("An error occurred while sending the message to the server.\n");
                        exit(0);
                    }
                    printf("(RE)SENT PKT: Seq No %d of size %d bytes from channel %d\n", send_pkt.sq_no, send_pkt.size, send_pkt.channel);
                    // printf("Packet %d sent successfully.\n", send_pkt.sq_no/BUFLEN);     
                }
                else
                {
                    bytes_recd = recv(client_socket1, &recv_pkt, sizeof(recv_pkt), 0);
                    if(bytes_recd < 0){
                        printf("An error occurred while receiving the message from the server.\n");
                        exit(0);
                    }
                    printf("RCVD ACK: for PKT with Seq No %d from channel %d\n", recv_pkt.sq_no, recv_pkt.channel);
                    break;
                }
            }    
        }
        sem_close(sema);
    }
    else
    {
        // client_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        // if(client_socket < 0){
        //     printf("An error occurred while creating the client socket\n");
        //     exit(0);
        // }

        // sleep(1);

        printf("The client socket %d (Channel 2) was successfully created.\n", client_socket2);       
        connection = connect(client_socket2, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
        if(connection < 0){
            printf("An error occurred while connecting client to server.\n");
            exit(0);
        }
        printf("Connection 2 established successfully.\n");
        
        DATA_PKT recv_pkt,send_pkt;
        while(1)
        {
            // int temp = ftell(fp);
            sem_wait(sem);
            send_pkt.size = fread(send_pkt.data ,1, BUFLEN, fp);
            send_pkt.sq_no = ftell(fp)-send_pkt.size;
            sem_post(sem);
            // if(send_pkt.sq_no - temp != 0)
            //     send_pkt.sq_no -= BUFLEN; 
            if(send_pkt.size == 0)
                break;
            // printf("BBBBBBBBBBB\n");
            send_pkt.data[send_pkt.size] = '\0';
            send_pkt.channel = 2;
            // printf("B:- Debug Data : %s\n", send_pkt.data);
            // printf("B:- SEQ NO : %d\n", send_pkt.sq_no);
            if(send_pkt.sq_no + BUFLEN < filesize)
            {
                send_pkt.isLast = 0;
            }
            else
            {
                // printf("XXXXXXXX\n");
                send_pkt.isLast = 1;
            }
            send_pkt.type = 0;

            // FD_ZERO(&readfds);
            // FD_SET(client_socket2, &readfds);

            // timeout.tv_sec = 2;
            // timeout.tv_usec = 0;

            bytes_sent = send(client_socket2, &send_pkt, sizeof(send_pkt), 0);
            if(bytes_sent != sizeof(send_pkt)){
                printf("An error occurred while sending the message to the server.\n");
                exit(0);
            }

            printf("SENT PKT: Seq No %d of size %d bytes from channel %d\n", send_pkt.sq_no, send_pkt.size, send_pkt.channel);
            while(1)
            {

                FD_ZERO(&readfds);
                FD_SET(client_socket2, &readfds);

                timeout.tv_sec = TIMEOUT;
                timeout.tv_usec = 0;

                int pkt_ack = select(client_socket2+1, &readfds, NULL, NULL, &timeout);
                if(pkt_ack <= 0)
                {
                    // Connection Timeout
                    // printf("Connection Timeout for Packet %d.\n", send_pkt.sq_no/BUFLEN);
                    bytes_sent = send(client_socket2, &send_pkt, sizeof(send_pkt), 0);
                    if(bytes_sent != sizeof(send_pkt)){
                        printf("An error occurred while sending the message to the server.\n");
                        exit(0);
                    }
                    printf("(RE)SENT PKT: Seq No %d of size %d bytes from channel %d\n", send_pkt.sq_no, send_pkt.size, send_pkt.channel);
            
                    // printf("Packet %d sent successfully.\n", send_pkt.sq_no/BUFLEN);     
                }
                else
                {
                    bytes_recd = recv(client_socket2, &recv_pkt, sizeof(recv_pkt), 0);
                    if(bytes_recd < 0){
                        printf("An error occurred while receiving the message from the server.\n");
                        exit(0);
                    }
                    printf("RCVD ACK: for PKT with Seq No %d from channel %d\n", recv_pkt.sq_no, recv_pkt.channel);
                    break;
                }
            }    
        }
        sem_close(sem);
        wait(NULL);
    }
    sem_unlink(SEM_NAME);
    close(client_socket1);
    close(client_socket2);
    return 0;
}