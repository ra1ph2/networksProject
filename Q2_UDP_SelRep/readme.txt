// Akshit Khanna 2017A7PS0023P
Problem 2: File transfer using Selective Repeat protocol over UDP

Files Included:
-- udpServer_exp.c
-- udpClient_exp.c
-- relay.c
-- packet.h

Methodology:

-- The client intially sends all packets in a window size of BUFSIZE (macro) to the server.
-- The client keeps a single timer for the whole window which resets at every ACK
   received for any packet in the window. A circular buffer is used as window. 
-- If the packet ack received is the base of the window,the window is shifted to the next 
   unacked packet or unsent packet, and packets which are now covered in this window 
   are also sent. At timeout all the unacked packets in the indow are resent.
-- The relay send the data packet to the server and ack packet to the client.
-- The server has a circular buffer to keep track of the window and and it writes the 
   contigous packets when base of the window is received and changes the base accordingly.

Execution:

-- The input file must be named "input.txt".
-- The output file created at server is named "out.txt"
-- The packet transmission data is both printed on the terminal as well as stored in log 
   files named "client_log.txt", "relay1_log.txt", "relay2_log.txt" and "server_log.txt".
-- relay.c takes command line argument '1' and '2' to create 2 relay servers.
-- Both client and server executable end on their own after execution but both the relays 
   need to be closed by the user on the terminal using CTRL + C.

The macros in packet.h are explained as following
-- PKT_SIZE - Size of payload from the files in bytes.
-- PROB - Percentage probability of server dropping the packet.
-- BUFSIZE - Size of window to limit packets flooding a single channel

Commands:

Terminal 1
-- gcc udpServer_exp.c -o server
-- ./server
Terminal 2
-- gcc relay.c -o rel
-- ./rel 1
-- (Need to close it after execution)
Terminal 3
-- gcc relay.c -o rel
-- ./rel 2
-- (Need to close it after execution)
Terminal 4
-- gcc udpClient_exp -o client
-- ./client

