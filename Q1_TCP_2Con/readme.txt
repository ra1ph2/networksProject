// Akshit Khanna 2017A7PS0023P
Problem 1: File transfer using multi-channel stop-and-wait protocol

Files Included:
-- tcpClient.c
-- tcpServer.c
-- packet.h

Methodology:

-- The client opens 2 TCP connections with the server before sending any packet.
-- Then fork() is used to create 2 processes, one each for the 2 connections.
-- Both process access shared file descriptor and keep on reading from the file 
   and create packets. Semaphores are also used for exclusive access of the shared
   file descriptor by both the processes.
-- There is buffer of size 4 (Its a macro by name of BUFSIZE in packet.h) which limits
   the packets sent if one of channels has a timeout because if timeout is large and 
   channel 1 is stuck at packet 1 and channel 2 sends the whole file to the server and
   it can't be written since packet 1 has not arrived.
-- The server uses a timer using select to select a connection for receiving the packet.
-- The server also has buffer to keep out of order packets with same BUFSIZE macro. 
-- The server keeps on writting to the output file as it gets a contigous packet stream
   from the base sequence.

Execution:

-- The file to sent needs to named "input.txt".
-- The file created at server is named "output_tcp.txt".
-- All the packet transmission data is printed on the terminals.

The macros in packet.h are explained as following
-- PKT_SIZE - Size of payload from the files in bytes.
-- PROB - Percentage probability of server dropping the packet.
-- BUFSIZE - Size of window to limit packets flooding a single channel

Commands:

Terminal 1
-- gcc tcpServer.c -o server
-- ./server
Terminal 2
-- gcc tcpClient.c -o client -lpthread
-- ./client