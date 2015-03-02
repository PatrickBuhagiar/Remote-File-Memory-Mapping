
#include "fmmap.h"
#include <stdio.h>
#define refuse -1
#define accept 0
#define requestRead 1
#define requestWrite 2
#define PORT 8088
#define IP 192.168.1.105
struct msg_t {
	int msg;
	char* data[];
}


main(){
	char str[100];
	//do{
		printf("Welcome to the program Client. \n");
		printf("***************************\n");
			//hardcoded
			char ip[15] = "127.0.0.1";
			char path[30] = "file.txt";
			int port = 3048;
			printf("***************************\n\nRequesting access to %s:%d for %s...\n\n", ip, port, path); 
			
			
			//Define location data and path name	
			fileloc_t *server =(struct fileloc*)malloc(sizeof(struct fileloc));
			server->ipaddress.s_addr =(unsigned long)ip;
			server->port = port;
			server->pathname = path;
		
			//RMMAP. offsets are hardcoded
			char * s;
			s = rmmap(*server, 0);
			
			//READ.
			char * buffer_t = malloc(1024); //buffer to hold read item. Count and offset are hardcoded 
			int ss; 
			ss = mread(s, 10, buffer_t ,40);
			printf("\nContent read from %d bits of memory mapped area %s\n", ss, buffer_t);
			   
			//WRITE
			int result = mwrite(s,20,buffer_t,40);			
					
			
			//printf("\nContent written from %d bits of memory mapped area %s\n", result, buffer_t);
		
			//int b = rmunmap(s);
		
	printf("Success\n");
	//free(s);
	
}	

