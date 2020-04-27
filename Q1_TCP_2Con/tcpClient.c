// Akshit Khanna 2017A7PS0023P
#include "packet.h"

int main()
{
    
    int client_socket;
	struct sockaddr_in serverAddress;	
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	
    int connection;
	int bytes_sent;
	int bytes_recd;

    int client_socket1 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(client_socket1 < 0){
        printf("An error occurred while creating the client socket\n");
        exit(0);
    }
      
    printf("The client socket %d (Channel 1) was successfully created.\n", client_socket1);       
    connection = connect(client_socket1, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    if(connection < 0){
        printf("An error occurred while connecting client to server.\n");
        exit(0);
    }
    printf("Connection 1 established successfully.\n");

    int client_socket2 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(client_socket2 < 0){
        printf("An error occurred while creating the client socket\n");
        exit(0);
    }

    printf("The client socket %d (Channel 2) was successfully created.\n", client_socket2);       
    connection = connect(client_socket2, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    if(connection < 0){
        printf("An error occurred while connecting client to server.\n");
        exit(0);
    }
    printf("Connection 2 established successfully.\n");

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
   
        sem_t *sema = sem_open(SEM_NAME, O_RDWR);
        DATA_PKT recv_pkt,send_pkt;
        while(1)
        {
            sem_wait(sema);
            send_pkt.size = fread(send_pkt.data ,1, PKT_SIZE, fp);
            send_pkt.sq_no = ftell(fp)-send_pkt.size;
            sem_post(sema);
            if(send_pkt.size == 0)
                break;
            send_pkt.data[send_pkt.size] = '\0';
            send_pkt.channel = 1;
            if(send_pkt.sq_no + PKT_SIZE < filesize)
            {
                send_pkt.isLast = 0;
            }
            else
            {
                send_pkt.isLast = 1;
            }
            send_pkt.type = 0;

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
                    bytes_sent = send(client_socket1, &send_pkt, sizeof(send_pkt), 0);
                    if(bytes_sent != sizeof(send_pkt)){
                        printf("An error occurred while sending the message to the server.\n");
                        exit(0);
                    }
                    printf("(RE)SENT PKT: Seq No %d of size %d bytes from channel %d\n", send_pkt.sq_no, send_pkt.size, send_pkt.channel);
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
        
        DATA_PKT recv_pkt,send_pkt;
        while(1)
        {
            sem_wait(sem);
            send_pkt.size = fread(send_pkt.data ,1, PKT_SIZE, fp);
            send_pkt.sq_no = ftell(fp)-send_pkt.size;
            sem_post(sem);
            if(send_pkt.size == 0)
                break;
            send_pkt.data[send_pkt.size] = '\0';
            send_pkt.channel = 2;
            if(send_pkt.sq_no + PKT_SIZE < filesize)
            {
                send_pkt.isLast = 0;
            }
            else
            {
                send_pkt.isLast = 1;
            }
            send_pkt.type = 0;

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
                    bytes_sent = send(client_socket2, &send_pkt, sizeof(send_pkt), 0);
                    if(bytes_sent != sizeof(send_pkt)){
                        printf("An error occurred while sending the message to the server.\n");
                        exit(0);
                    }
                    printf("(RE)SENT PKT: Seq No %d of size %d bytes from channel %d\n", send_pkt.sq_no, send_pkt.size, send_pkt.channel);
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