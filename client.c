#include <string.h>
#include <sys/socket.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <stdio.h>
#include <stdlib.h> 
#include "client_functions.h"	 
#define PORT 8000
//MT2021057 ISHAN ARORA
//TRAIN RESERVATION PROJECT



int main(void) { 
	int sock; 
    	struct sockaddr_in server; 
    	char server_reply[50],*server_ip;
	server_ip = "127.0.0.1"; 
     
    	sock = socket(AF_INET, SOCK_STREAM, 0); 
    	if (sock == -1) { 
       	printf("Could not create socket"); 
    	} 
    
    	server.sin_addr.s_addr = inet_addr(server_ip); 
    	server.sin_family = AF_INET; 
    	server.sin_port = htons(PORT); 
   
    	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0)//socket-->connect(to connect to server)
       	perror("connect failed. Error"); 
    
	while(client(sock)!=3);//client function to show the first introduction page
    	close(sock); 
    	
	return 0; 
} 

