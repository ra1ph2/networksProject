// Akshit Khanna 2017A7PS0023P
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
     
#define MAX_CON 2     
#define PORT 8888
#define PKT_SIZE 100
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
    char data[PKT_SIZE+1];
}DATA_PKT;
