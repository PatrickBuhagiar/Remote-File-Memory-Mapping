#ifndef FMMAP
#define FMMAP
#include <netinet/in.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
 
/* PROTOCOL
*       rmmap -> 1|offset|pathname
*       read -> 2|start|count|pathname
*       write -> 3|start|buffer|pathname
*       rmunmap -> 4|offset|length|pathname
*/
 
 
 
 
// Type definition for remote file location
struct fileloc {
    struct in_addr ipaddress; // Remote IP
    int port;                 // Remote port
    char *pathname;           // Remote file to be memory mapped
};
typedef struct fileloc fileloc_t;


//Packet used  
#pragma pack(1)
typedef struct RMPACKET
{
        int type; //type of message: map, unmap, read, write
        long iOne; //var 1
        long iTwo; //var 2
        char pathName[1000]; //pathname
        char * data;
} RMP;
#pragma pack(0)

//Global viariables 
int s = 0; //socket
off_t foffset = 0; //file offset
fileloc_t loc; //location data
 
 
// Create a new memory mapping in the virtual address space
// of the calling process        
// location: Remote file to be memory mapped
// offset:   File offset to memory map (from beginning of file)
//y
// returns: Pointer to the mapped area. On error the value MAP_FAILED
//          ((void*) -1) is returned and errno is set appropriately
void *rmmap(fileloc_t location, off_t offset){
        //set global parameters
        foffset = offset;
        loc = location;

	//initialisations        
        int w, t, len;
        char buf[1024];
        void * r;
        char * buffer;
        bzero(buf,1024);
        RMP msg;
        struct sockaddr_in remote;
       
       //condition only for preventing recreating a socket when one already exists
        if (s == 0){
                //Create socket
                if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                        perror("socket");
                        exit(1);
                }
       
                bzero(&remote,sizeof(remote));
                remote.sin_family = AF_INET;
		remote.sin_port = htons(3048);
		remote.sin_addr.s_addr = inet_addr("127.0.0.1");
                int result;
       
                //Connect
                result = connect(s, (struct sockaddr *)&remote, sizeof(remote));
                if (result == 0) {
                        printf("\n   *****\nClient Connected \n    *****\n");
                } else {
                        perror("connect");
                        exit(1);
                }      
        }
       
        //Build packet
        msg.type = 1;
        msg.iOne = offset;
        msg.iTwo = 0;
        strcpy(msg.pathName, location.pathname);
       
        //Send request
        int b;
        b = send(s, (void*)&msg, sizeof(struct RMPACKET), 0);
        printf("\nB: %d\n", b);
       
        buffer = (char*)malloc (sizeof(char)*1024);
        bzero(buffer, sizeof(buffer));
       
        //Receive Page
        w = recv(s, buffer, 1024, 0);
        if(w <0){
             	perror("receive");
     		return 0;
     	}
        printf("Received page of %d bits.\n",w);
       
        r = buffer;
        return r;
       
}
 
// Deletes mapping for the specified address range
// addr:   Address returned by mmap
// length: Length of mapping
//
// returns: 0 on success, -1 on failure and errno set appropriately
int rmunmap(void *addr){
        RMP msg;
       
       	//create message for server
        msg.type = 4;
        msg.iOne = foffset;
        msg.iTwo = strlen(addr);
        strcpy(msg.pathName, loc.pathname);
               
        //Send request
        int b;
        b = send(s, (void*)&msg, sizeof(struct RMPACKET), 0);
       
       	//close socket
        close(s);
        s=0;
       	
        return 0;
}
 
// Attempt to read up to count bytes from memory mapped area pointed
// to by addr + offset into buff
// addr: page read from rmap
// returns: Number of bytes read from memory mapped area
ssize_t mread(void *addr, off_t offset, void *buff, size_t count){
        
        //Initialisations
        RMP msg;
        int start = (int)foffset + (int)offset;
        int end = start + (int)count;
        int back = foffset + strlen((char*)addr);
        
        //Just to give a good clear idea of what offset's we are deling with
        printf("\nfile offset: %d Offset: %d\n", (int)foffset, (int)offset);
        printf("Start: %d Count: %d End: %d\n",start, (int)count, end);
        printf("Back: %d, size of page %d\n", back, strlen((char*)addr));
        printf("Contents: \n\n%s\n\n", (char*)addr);
       	
       	
        //Case we need to request another page since the client requested a larger read..
        if(start>back || end > back){
                if (end > back){
                       
                        //determine number of extra pages to be read
                        int i = (int)(((end-back)/1024) + 1);
                        printf("\nWe require %d more pages!\n", i);
                       
                        int j = 0;
                       
                        char new_m[strlen(addr) + 1024 * (i)]; //content of all mapped pages
                        bzero(new_m,strlen(new_m));
                        strcat(new_m, addr);
                       
                        //loop for # of extra pages to be added by map
                        while(j<i){
                                char * temp;
                                int offset = foffset + (1024 * j);
                                temp = rmmap(loc, offset);
                                strcat(new_m, temp);
                                free(temp);
                                j++;
                        }
                       
                        printf("\nThe size of addr will now be \n %d bits\n", strlen(new_m));
                        
                        //addr stores mapped pages
                        strcpy(addr,new_m);
                
                } else if (start>back){
                        printf("Access denied. Requested read is outside mapped boundaries.\n");
                        return -1;
                }
        }
        
        //build message for server
        msg.type = 2;
        msg.iOne = start;
        msg.iTwo = count;
        strcpy(msg.pathName, loc.pathname);
       
        //Send request
        int b;
        b = send(s, (void*)&msg, sizeof(struct RMPACKET), 0);
                     
        //temporary buffer
        char buf[count+1];
        strncpy(buf, addr + offset, count+1);
      	buf[count+1] = '\0';
        
        //Update buffer in client
      	strcpy(buff,buf);
       	
       	
        return count;      
}
 
// Attempt to write up to count bytes to memory mapped area pointed
// to by addr + offset from buff
//
// returns: Number of bytes written to memory mapped area
ssize_t mwrite(void *addr, off_t offset, void *buff, size_t count){
        RMP msg;
        int start = (int)foffset + (int)offset;
       
        msg.type = 3;
      	msg.iOne = start;
        msg.iTwo = count;
        strcpy(msg.pathName, loc.pathname);
       	msg.data = buff;
       	
       	printf("hello %s",msg.data);
       
        int b;
    	b = send(s, (void*)&msg, sizeof(struct RMPACKET), 0);
	printf("sent message\n");
	
		return 0;
}
#endif
