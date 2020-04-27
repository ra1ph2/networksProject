#include <stdio.h>  
#include <string.h>    
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>     
#include <arpa/inet.h>   
#include <sys/types.h>  
#include <sys/wait.h>
#include <sys/stat.h>    
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/time.h>   
#include <time.h>
     
#define PORT_RELAY1 8001
#define PORT_RELAY2 8002
#define PORT_SERVER 8003
#define PKT_SIZE 10
#define PROB 10  
#define BUFSIZE 4

typedef struct packet{
    int size;
    int sq_no;
    int type; // 0 :- DATA  , 1 :- ACK
    int isLast; // 1 :- Last Packet
    char data[PKT_SIZE+1];
}DATA_PKT;