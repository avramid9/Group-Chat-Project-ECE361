#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

struct packet {  
    unsigned int total_frag;  
    unsigned int frag_no; 
    unsigned int size; 
    char* filename; 
    char filedata[1000];  
};

char* packet_to_message(struct packet p, int* message_len){
    //Get strings and their length from first 3 fields in packet
    int total_frag_len = snprintf(NULL, 0, "%d", p.total_frag);
    char* total_frag = malloc(total_frag_len + 1);
    snprintf(total_frag, total_frag_len+1, "%d", p.total_frag);

    int frag_no_len = snprintf(NULL, 0, "%d", p.frag_no);
    char* frag_no = malloc(frag_no_len + 1);
    snprintf(frag_no, frag_no_len+1, "%d", p.frag_no);

    int size_len = snprintf(NULL, 0, "%d", p.size);
    char* size = malloc(size_len + 1);
    snprintf(size, size_len+1, "%d", p.size);
    
    //Allocate char array for the message which will be returned as a pointer
    int mSize = 4 + total_frag_len + frag_no_len + size_len + strlen(p.filename) + p.size; 
    char *m = malloc(mSize);
    char test = (char) p.total_frag;

    //Keep track of position each time we add a string to the message
    int mPos = 0;

    //Add first three variables to message
    for(int i = mPos; i < (mPos + total_frag_len); i++){
        m[i] = total_frag[i-mPos];
    }
    m[mPos + total_frag_len] = ':';
    mPos += total_frag_len+1;

    for(int i = mPos; i < (mPos + frag_no_len); i++){
        m[i] = frag_no[i-mPos];
    }
    m[mPos + frag_no_len] = ':';
    mPos += frag_no_len+1;

    for(int i = mPos; i < (mPos + size_len); i++){
        m[i] = size[i-mPos];
    }
    m[mPos + size_len] = ':';
    mPos += size_len+1;

    //Add filename to message
    for(int i = mPos; i < mPos+strlen(p.filename); i++){
        m[i] = p.filename[i-mPos];
    }
    m[mPos + strlen(p.filename)] = ':';
    mPos += strlen(p.filename)+1;

    //Add file data to message
    for(int i = mPos; i < mSize; i++){
        m[i] = p.filedata[i-mPos];
    }

    free(total_frag);
    free(frag_no);
    free(size);
    
    *message_len = mSize;
    return m;
}

int main(int argc, char *argv[]){
    int socketFD = socket(AF_INET,SOCK_DGRAM,0);
    
    //Check for 2 arguments
    if (argc != 3){
        printf("Incorrect number of arguments\n");
        exit(1);
    }

    //Get arguments
    int port = atoi(argv[2]);
    char *ipAddress = argv[1];

    char fileLoc[100];
    char ftpCheck[10];
    printf("Input a message of the following form: ftp <file name>\n");
    //Check for ftp command
    scanf("%s", &ftpCheck);
    if(strcmp(ftpCheck, "ftp") != 0){
        printf("Incorrect command\n");
        exit(1);
    }
    //Get file location
    scanf("%s", &fileLoc);

    //Check if file exists
    if(access(fileLoc, F_OK) != 0){
        printf("That file does not exist\n");
        exit(1);
    }
    
    //read file + create packets/1000 byte
    //check file size first
    struct stat st;
    stat(fileLoc, &st);
    
    
    int total_frag = st.st_size/1000+1;
    int frag_no=1;
    
    struct packet packet_array[100]; //used to store packets we need to send
    char temp_data[1000];
    
    int fd_from;  
    ssize_t reader;
    
    if((fd_from=open(fileLoc, O_RDWR))<0)
        printf("error reading file");   
    while((reader=read(fd_from,packet_array[frag_no].filedata,1000))>0){ 
        packet_array[frag_no].total_frag = total_frag;
        packet_array[frag_no].frag_no = frag_no;
        packet_array[frag_no].filename = fileLoc;
        if(frag_no!=total_frag)
            packet_array[frag_no].size = 1000;
        else
            packet_array[frag_no].size = st.st_size%1000;

        
        //increment fragment number
        frag_no++;
    }
    
    //close file since we're done reading
    if(close(fd_from)<0)
        printf("error closing file");
    
    
    
    //Put server information into sockaddr_in
    struct sockaddr_in sa;
    memset(&sa.sin_zero, 0, 8*sizeof(unsigned char));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress, &(sa.sin_addr));

    ssize_t bytesSent;
    
    //continue ...
    
    
    //Send ftp message to server
    struct sockaddr_storage sa_stor;
    socklen_t sa_stor_size = sizeof(sa_stor);
    ssize_t bytesReceived;

    int bufLen = 10;
    char buf[bufLen];
    
    for(int i=1; i<=frag_no;i++){
        int length;
        char* message_s = packet_to_message(packet_array[i],&length);
        if((bytesSent = sendto(socketFD, message_s, length, 0, (struct sockaddr *) &sa, sizeof(sa))) == -1){
            printf("Error sending message to server\n");
            exit(1);
        }
        
        if((bytesReceived = recvfrom(socketFD, buf, bufLen, 0, (struct sockaddr *) &sa_stor, &sa_stor_size)) == -1){
            printf("Error receiving message from server");
            exit(1);
        }
        
    }
    

    //Receive message from server
    if((bytesReceived = recvfrom(socketFD, buf, bufLen, 0, (struct sockaddr *) &sa_stor, &sa_stor_size)) == -1){
        printf("Error receiving message from server");
        exit(1);
    }
    
    
    //Check return message from server
    if(strcmp(buf, "completed") != 0){
        exit(1);
    }

    printf("File transfer completed\n");
    close(socketFD);
     
    return 0;
}
