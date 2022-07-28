#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "serv_functions.h"
#define PORT 8000
//MT2021057 ISHAN ARORA
//TRAIN RESERVATION PROJECT


   
int main(void) { //setup of server and functionality of concurrent clients
 
    int socket_desc, client_sock, c; 
    struct sockaddr_in server, client; 
    char buf[100]; 
  
    socket_desc = socket(AF_INET, SOCK_STREAM, 0); //socket-->bind-->listen-->accept
    if (socket_desc == -1) { 
        printf("Could not create required socket"); 
    } 
  
    server.sin_family = AF_INET; 
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT); 
   
    if (bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0) 
        perror("bind failed. Error"); 
   
 
    listen(socket_desc, 1);  
    c = sizeof(struct sockaddr_in); 
  
    while (1){

	    client_sock = accept(socket_desc, (struct sockaddr*)&client, (socklen_t*)&c); 
	  
	    if (!fork()){
		    close(socket_desc);//closing the previous server socket to service the new client one
		    service_cli(client_sock);							
		    exit(0);
	    }
	    else
	    	close(client_sock);//after the client has been executed and finished i.e exit the parent can close client child socket 
    }
    return 0;
}
