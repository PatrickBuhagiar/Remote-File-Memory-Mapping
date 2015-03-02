/* Server File for the Shared Memory Mapping Program
* 
* Table of Contents
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
            
#define IP "127.0.0.1"
#define PORT 3048
#define CLIENTS 1000    
#define BUFFSIZE 1023
     
    #pragma pack(1)
    typedef struct RMPACKET { int type; long iOne; long iTwo; char pathName[1000]; char * data; }RMP;
    #pragma pack(0)
             
    struct READ { int rid; int tid; int fid; int timestamp; long start; long end; };
             
    struct READNODE { struct READ read_info; struct READNODE *next; };
             
    struct READLIST { struct READNODE *head, *tail; int size; int ridgen; };
             
    struct FILE { int fid; char pathname[1024]; long floor; long ceiling; char *filedata; int timestamp; };
             
    struct FILENODE { struct FILE src; struct FILENODE *next; };
             
    struct FILELIST { struct FILENODE *head, *tail; int size; int fidgen; };
             
    struct THREADINFO { pthread_t thread_ID; int sockfd; };
             
    struct CLIENTONDE { struct THREADINFO threadinfo; struct CLIENTONDE *next; };
             
    struct CLIENTLIST { struct CLIENTONDE *head, *tail; int size; };
     
    int FileCompare(struct FILE *a, struct FILE *b) {
            printf("The first fid is %d\n", a->fid);
            printf("The Second fid is %d\n", b->fid);
            return a->fid == b->fid;
            }
     
    struct FILE* MakeFile(long f, long c, char* n, char* datapointer){
            struct FILE* new = (struct FILE*)malloc(sizeof(struct FILE));
            new->floor = f;
            new->ceiling = c;
            strcpy(new->pathname,n);
            new->filedata = (char *) malloc(sizeof(char) * strlen(datapointer));
            strcpy(new->filedata, datapointer);
            return new;
            }
             
    void FileList_init(struct FILELIST *fl) {
            fl->head = fl->tail = NULL;
            fl->size = 0;
            fl->fidgen = 0;
            }
             
    int FileList_insert(struct FILELIST *fl, struct FILE *somefile) {
            fl->fidgen++;
            somefile->fid = fl->fidgen;
            if(fl->head == NULL) {
                    fl->head = (struct FILENODE *)malloc(sizeof(struct FILENODE));
                    fl->head->src = *somefile;
                    fl->head->next = NULL;
                    fl->tail = fl->head;
            } else {
                    fl->tail->next = (struct FILENODE *)malloc(sizeof(struct FILENODE));
                    fl->tail->next->src = *somefile;
                    fl->tail->next->next = NULL;
                    fl->tail = fl->tail->next;
            }
            fl->size++;
            return 0;
    }
             
    int FileList_delete(struct FILELIST *fl, struct FILE *somefile) {
            struct FILENODE *curr, *temp;
            if(fl->head == NULL) { return -1; }
            if(compare(somefile, &fl->head->src) == 0) {
                    temp = fl->head;
                    fl->head = fl->head->next;
                    if(fl->head == NULL) fl->tail = fl->head;
                    free(temp);
                    fl->size--;
                    return 0;
            }
            for(curr = fl->head; curr->next != NULL; curr = curr->next) {
                    if(compare(somefile, &curr->next->src) == 0) {
                    temp = curr->next;
                            if(temp == fl->tail) fl->tail = curr;
                            curr->next = curr->next->next;
                            free(temp);
                            fl->size--;
                            return 0;
                    }
            }
            return -1;
    }
             
    void FileList_dump(struct FILELIST *fl) {
            struct FILENODE *curr;
            struct FILE *somefile;
            printf("File count: %d\n", fl->size);
            for(curr = fl->head; curr != NULL; curr = curr->next) {
                    somefile = &curr->src;
                    printf("(%d) %s :: %lu to %lu at %d\n", somefile->fid, somefile->pathname, somefile->floor, somefile->ceiling, somefile->timestamp);
            }
    }
             
    int ReadList_compare(struct READ *a, struct READ *b) { return (a->rid == b->rid); }
             
    struct READ* MakeRead(int thread, int file, long s, long e){
            struct READ* new = (struct READ*)malloc(sizeof(struct READ));
            new->tid = thread;
            new->fid = file;
            new->start = s;
            new->end = e;
            new->timestamp = time(NULL);
            return new;
    }
             
    void ReadList_init(struct READLIST *rl) {
            rl->ridgen = -1;
            rl->head = rl->tail = NULL;
            rl->size = 0;
    }
             
    int ReadList_insert(struct READLIST *rl, struct READ *read_info) {
            rl->ridgen++;
            read_info->rid = rl->ridgen;
            if(rl->head == NULL) {
                    rl->head = (struct READNODE *)malloc(sizeof(struct READNODE));
                    rl->head->read_info = *read_info;
                    rl->head->next = NULL;
                    rl->tail = rl->head;
            } else {
                    rl->tail->next = (struct READNODE *)malloc(sizeof(struct READNODE));
                    rl->tail->next->read_info = *read_info;
                    rl->tail->next->next = NULL;
                    rl->tail = rl->tail->next;
                }
                rl->size++;
                return 0;
    }
             
            int ReadList_delete(struct READLIST *rl, struct READ *read_info) {
                struct READNODE *curr, *temp;
                if(ReadList_compare(read_info, &rl->head->read_info) == 0) {
    temp = rl->head;
                    rl->head = rl->head->next;
    if(rl->head == NULL) rl->tail = rl->head;
    free(temp);
    rl->size--;
    return 0;
                }
                for(curr = rl->head; curr->next != NULL; curr = curr->next) {
    if(ReadList_compare(read_info, &curr->next->read_info) == 0) {
        temp = curr->next;
        if(temp == rl->tail) rl->tail = curr;
        curr->next = curr->next->next;
        free(temp);
        rl->size--;
        return 0;
    }
                }
                return -1;
            }
             
            void ReadList_dump(struct READLIST *rl) {
                struct READNODE *curr;
                struct READ *read_info;
                printf("Connection count: %d\n", rl->size);
                for(curr = rl->head; curr != NULL; curr = curr->next) {
    read_info = &curr->read_info;
    printf("[%d] Thread %d, Read File %d, From %lu to %lu, At %d\n", read_info->rid, read_info->tid, read_info->fid, read_info->start, read_info->end, read_info->timestamp);
                }
            }
             
            int compare(struct THREADINFO *a, struct THREADINFO *b) {
                return a->sockfd - b->sockfd;
            }
             
            void ClientList_init(struct CLIENTLIST *ll) {
                ll->head = ll->tail = NULL;
                ll->size = 0;
            }
             
            int ClientList_insert(struct CLIENTLIST *ll, struct THREADINFO *thr_info) {
                if(ll->size == CLIENTS) return -1;
                if(ll->head == NULL) {
                    ll->head = (struct CLIENTONDE *)malloc(sizeof(struct CLIENTONDE));
                    ll->head->threadinfo = *thr_info;
                    ll->head->next = NULL;
                    ll->tail = ll->head;
                }
                else {
                    ll->tail->next = (struct CLIENTONDE *)malloc(sizeof(struct CLIENTONDE));
                    ll->tail->next->threadinfo = *thr_info;
                    ll->tail->next->next = NULL;
                    ll->tail = ll->tail->next;
                }
                ll->size++;
                return 0;
            }
             
            int ClientList_delete(struct CLIENTLIST *ll, struct THREADINFO *thr_info) {
                struct CLIENTONDE *curr, *temp;
                if(ll->head == NULL) return -1;
                if(compare(thr_info, &ll->head->threadinfo) == 0) {
                    temp = ll->head;
                    ll->head = ll->head->next;
                    if(ll->head == NULL) ll->tail = ll->head;
                    free(temp);
                    ll->size--;
                    return 0;
                }
                for(curr = ll->head; curr->next != NULL; curr = curr->next) {
                    if(compare(thr_info, &curr->next->threadinfo) == 0) {
                        temp = curr->next;
                        if(temp == ll->tail) ll->tail = curr;
                        curr->next = curr->next->next;
                        free(temp);
                        ll->size--;
                        return 0;
                    }
                }
                return -1;
            }
             
            void ClientList_dump(struct CLIENTLIST *ll) {
                struct CLIENTONDE *curr;
                struct THREADINFO *thr_info;
                printf("********* THREAD LIST DUMP *********\n");
                for(curr = ll->head; curr != NULL; curr = curr->next) {
                    thr_info = &curr->threadinfo;
                    printf("Client [%d] \n", thr_info->sockfd);
                }
            }
             
            /* Global Variables */
             
            int sockfd, newfd;
            struct THREADINFO thread_info[CLIENTS];
            struct CLIENTLIST client_list;
            struct FILELIST file_list;
            struct READLIST read_list;
            pthread_mutex_t clientlist_mutex;
            pthread_mutex_t filelist_mutex;
            pthread_mutex_t readlist_mutex;
     
            void *client_handler(void *fd);
            void *io_handler(void *param);
             
            int main(int argc, char **argv) {
                int err_ret, sin_size;
                struct sockaddr_in serv_addr, client_addr;
                pthread_t interrupt;
             
            /* Initiialize Data Structures and Mutexes */
            ClientList_init(&client_list); FileList_init(&file_list); ReadList_init(&read_list);
            pthread_mutex_init(&clientlist_mutex, NULL); pthread_mutex_init(&filelist_mutex,NULL);
            pthread_mutex_init(&readlist_mutex,NULL);
             
                if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { perror("error"); }
             
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons(PORT);
                serv_addr.sin_addr.s_addr = inet_addr(IP);
                memset(&(serv_addr.sin_zero), 0, 8);
             
                if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1) { perror("error"); }
             
                if(listen(sockfd, 10) == -1) {      perror("error"); }
     
                if(pthread_create(&interrupt, NULL, io_handler, NULL) != 0) { error("error"); }
             
                printf("Server: Listening...\n");
                while(1) {
                    printf("ACCEPT LOOP STILL RUNNING\n");
                    sin_size = sizeof(struct sockaddr_in);
                    if((newfd = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t*)&sin_size)) == -1) {
                        perror("error");
                    }
                    else {
                        if(client_list.size == CLIENTS) {
                            fprintf(stderr, "Connection full, request rejected...\n");
                            continue;
                        }
                        printf("Connection requested received...\n");
                        struct THREADINFO threadinfo;
                        threadinfo.sockfd = newfd;
                        pthread_mutex_lock(&clientlist_mutex);
                        ClientList_insert(&client_list, &threadinfo);
                        pthread_mutex_unlock(&clientlist_mutex);
                        pthread_create(&threadinfo.thread_ID, NULL, client_handler, (void *)&threadinfo);
                    }
                }
                return 0;
            }
             
            void *io_handler(void *param) { return 0; }
     
            void *client_handler(void *fd) {
                struct THREADINFO threadinfo = *(struct THREADINFO *)fd;
                RMP packet;
                struct CLIENTONDE *curr;
                struct FILENODE *currentfile;
                struct READNODE *currentread;
                int bytes;
                while(1) {
                    printf("Server: [%d] Waiting for Another Message...\n",threadinfo.sockfd);
                    bytes = recv(threadinfo.sockfd, (void *)&packet, sizeof(struct RMPACKET), 0);
                    if(!bytes) {
                        struct READ * someread;
                        fprintf(stderr, "Connection lost from [%d]\n", threadinfo.sockfd);
                        for(currentread = read_list.head; currentread != NULL; currentread = currentread->next){
                                            if(currentread->read_info.tid == threadinfo.sockfd){
                                             someread = &currentread->read_info;      
                                             pthread_mutex_lock(&readlist_mutex);
                                             ReadList_delete(&read_list, someread);
                                             pthread_mutex_unlock(&readlist_mutex);
                                            }
                                    }
         
                        pthread_mutex_lock(&clientlist_mutex);
                        ClientList_delete(&client_list, &threadinfo);
                        pthread_mutex_unlock(&clientlist_mutex);
                        break;
                    }
                    if(packet.type == 1){
                             printf("Server: [%d] RECV MMAP %d|%lu|%lu|%s \n", threadinfo.sockfd, packet.type, packet.iOne, packet.iTwo,packet.pathName);
                    long floor_pointer = packet.iOne;
                    long ceiling_pointer = floor_pointer + BUFFSIZE;
                    char *path = malloc(sizeof(char) * strlen(packet.pathName));
                    strcpy(path,packet.pathName);
                    char* newdata;
                    int fd = open(path,O_RDWR);
                    if(fd < 0) {
                            send(threadinfo.sockfd,"-1",3,0);
                            printf("Server: Error File Cannot Be Found/Opened!");
                    } else {
                            pthread_mutex_lock(&filelist_mutex);
                            int found = 0;
                            newdata = mmap((caddr_t)0, ceiling_pointer - floor_pointer, PROT_WRITE, MAP_SHARED, fd, 0);
                            struct FILE * newfile;
                            for(currentfile = file_list.head; currentfile != NULL; currentfile = currentfile->next) {
                                    if(strcmp(currentfile->src.pathname,path) == 0) {
                            if((currentfile->src.floor > floor_pointer) && (currentfile->src.ceiling < ceiling_pointer)){
                                                    currentfile->src.floor = floor_pointer;
                                                    currentfile->src.ceiling = ceiling_pointer;
                                                    } else if(currentfile->src.floor > floor_pointer){
                                                            currentfile->src.floor = floor_pointer;
                                                    } else if(currentfile->src.ceiling < ceiling_pointer){
                                                            currentfile->src.ceiling = ceiling_pointer;
                                                    }
                                                    currentfile->src.filedata = (char *) malloc(sizeof(char) * strlen(newdata));
                                                    strcpy(currentfile->src.filedata, newdata);
                                                    //currentfile->src.filedata = newdata;
                                                    currentfile->src.timestamp = time(NULL);
                                                    found = 1;
                                            }
                                    }
                                    if(found == 0){
                                                newfile = MakeFile(floor_pointer,ceiling_pointer,path,newdata);                                
                                                FileList_insert(&file_list, newfile);
                                            if(send(threadinfo.sockfd,newfile->filedata, strlen(newfile->filedata),0) > 0){
                                                printf("Server: Sent Some Data(%lu to %lu for %s \n",floor_pointer,ceiling_pointer,path);
                                            }
                                    }
                                    FileList_dump(&file_list);
                                    pthread_mutex_unlock(&filelist_mutex);      
                            }
                    }
                    else if(packet.type == 2){
                                printf("Server: [%d] RECV READ %d|%lu|%lu|%s \n", threadinfo.sockfd, packet.type, packet.iOne, packet.iTwo,packet.pathName);
                                long m_start = packet.iOne;
                                long m_count = packet.iTwo;
                                char *path = malloc(sizeof(char) * strlen(packet.pathName));
                                strcpy(path,packet.pathName);
                                int found = 0;
                                char returnmsg[1024];
                                struct READ * newread;
                                printf("REC PARSING -> Floor: %lu Ceiling: %lu Path: %s\n",m_start, m_start + m_count,path);
                                pthread_mutex_lock(&filelist_mutex);
                                for(currentfile = file_list.head; currentfile != NULL; currentfile = currentfile->next) {
                                        if(strcmp(currentfile->src.pathname, path) == 0) {
                                        printf("REC -> FOUND FILE\n");
                                        if(m_start < currentfile->src.floor || (m_start + m_count) > currentfile->src.ceiling){
                                                     perror(">> An Error has Occured");
                                                     //send(threadinfo.sockfd,"SomeStuff",9,0);
                                                    break;
                                                    } else {
                                                    printf("REC -> DID SOME STUFF HERE \n");
                                                    pthread_mutex_lock(&readlist_mutex);
                                                    newread = MakeRead(threadinfo.sockfd,currentfile->src.fid,m_start,m_start+m_count);
                                                    ReadList_insert(&read_list,newread);
                                                    pthread_mutex_unlock(&readlist_mutex);
                                                    }
                                                    found == 1;
                                                    break;
                                            }
                            }
                            pthread_mutex_unlock(&filelist_mutex);
                    }
                    else if(packet.type == 3){
                            printf("Server: [%d] RECV WRITE %d|%lu|%lu|%s \n", threadinfo.sockfd, packet.type, packet.iOne, packet.iTwo,packet.pathName);
                           // printf("\n\nPCKT DATAAAA%s\n\n", packet.data);
                            int m_start = packet.iOne;
                            int m_count = packet.iTwo;
                            char *path = malloc(sizeof(char) * strlen(packet.pathName));
                            strcpy(path,packet.pathName);
                            int error = 1;
                            int found = 0;
                            // CREATE FILE POINTER AND FIND THE FILE
                            int filefound = 0;
                            struct FILE * thefile;
                            pthread_mutex_lock(&filelist_mutex);
                            pthread_mutex_lock(&readlist_mutex);
                            for(currentfile = file_list.head; currentfile != NULL; currentfile = currentfile->next){
                                    if(strcmp(currentfile->src.pathname, path)){
                                            thefile = &currentfile->src;
                                            filefound = 1;
                                            break;
                                    }
                            }
                            printf(">>>>> THE FILE HAS BEEN FOUND FOR THE WRITE!\n\n");
                            if(filefound != 0){
                                    for(currentread = read_list.head; currentread != NULL; currentread = currentread->next){
                                            printf("CHECK %d %d", currentread->read_info.tid , threadinfo.sockfd);
                                            if(currentread->read_info.tid == threadinfo.sockfd && currentread->read_info.fid == thefile->fid){
    // (currentread->read_info.tid == threadinfo.sockfd && currentread->read_info.fid == thefile->fid && currentread->read_info.start <= m_start && thefile->ceiling <= (m_start + m_count)
                                            printf("Test 1\n");
                                             // Found the read
                                                    if(currentread->read_info.timestamp >= thefile->timestamp){
                                            printf("Test 2\n");
                                                    // Timestamp is ok
                                                            if(m_start >= thefile->floor && (m_start + m_count) <= thefile->ceiling){
                                            printf("Test 3\n");                                                        
                                                    // Write is within Memory... so ok
                                                                    if(ReadList_delete(&read_list,&currentread->read_info)){
                                                                             thefile->timestamp = time(NULL);
                                                                             error = 0;
                                                                    }
                                                            }
                                                    }
                                            break;
                                            }
                                    }
                            } else {
                                    error = 1;
                            }
                            pthread_mutex_unlock(&readlist_mutex);
                            pthread_mutex_unlock(&filelist_mutex);
                            printf(">>>>> GOT TO THE REAL STUFF\n\n");
                            if(error = 1){ // Send Error Message
                            } else {
                            struct flock fl;
                            int fd;
                            fd = open(path, O_WRONLY);
                            if(fd < 0) { perror(">> File Does Not Exist)"); } else {
                                    int w;
                                    int num;
                                    //send(threadinfo.sockfd,"xxxxx", strlen("xxxxx"),0);
                                    char buf[m_start + m_count];
                                    //w = recv(threadinfo.sockfd, buf, strlen(buf), 0);
                                            fl.l_type   = F_WRLCK;
                                            fl.l_whence = SEEK_SET;
                                            fl.l_start  = m_start;
                                            fl.l_len    = m_start + m_count;
                                            fl.l_pid    = getpid();
                                            fcntl(fd, F_SETLKW, &fl);
                                            
                                            if ((num = write(fd, buf, strlen(buf))) == -1){
                                                    if(send(threadinfo.sockfd, "0", strlen("0"), 0) > 0){
                                                            perror(">> Write Failed");
                                                    }
                                            } else {
                                                    if(send(threadinfo.sockfd, "1", strlen("1"), 0) > 0){
                                                            printf("Server: Write of %d Bytes Successful.", num);
                                                    }
                                            }
                                            fl.l_type = F_UNLCK;
                                            fcntl(fd, F_SETLK, &fl);
                                           
                                    }
                            }
                    }
             
                    else if(packet.type == 4){
                            printf("Server: [%d] RECV UNMAP %d|%lu|%lu|%s \n", threadinfo.sockfd, packet.type, packet.iOne, packet.iTwo,packet.pathName);
                            long m_offset = packet.iOne;
                            long m_pagesize = packet.iTwo;
                            char *path = malloc(sizeof(char) * strlen(packet.pathName));
                            strcpy(path,packet.pathName);
                            int reply;
                            reply = 1;
                            reply = 0;
                            printf("Offset: %lu , Pagesize: %lu, Filename: %s",m_offset,m_pagesize,path);
                    }
                    else if(packet.type == 5){
                            break;
                    }
                    else {
                            printf("Recieved Something funny! : %d \n", packet.type);
                    }
                }
                close(threadinfo.sockfd);
             
                return NULL;
            }


