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

char* packet_to_single(struct packet p){
    //idk how to concatenate without using string functions ree
    char* k = p.total_frag + ":" + p.frag_no + ":" + p.size + ":" + p.filename + ":" + p.filedata;
    return k;
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
    int size = st.st_size;
    
    int total_frag = st.st_size/1000+1;
    int frag_no=1;
    
    struct packet packet_array[100]; //used to store packets we need to send
    char temp_data[1000];
    
    int fd_from;
    
    ssize_t reader;
    if((fd_from=open(fileLoc, O_RDONLY))<0)
        printf("error reading file");   
    while((reader=read(fd_from,temp_data,sizeof(temp_data)))>0){ 
        packet_array[frag_no].total_frag = total_frag;
        packet_array[frag_no].frag_no = frag_no;
        packet_array[frag_no].filename = fileLoc;
        packet_array[frag_no].size = size;
        packet_array[frag_no].filedata = temp_data;
        
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
    
    /*
    //Send ftp message to server
    if((bytesSent = sendto(socketFD, "ftp", sizeof("ftp"), 0, (struct sockaddr *) &sa, sizeof(sa))) == -1){
        printf("Error sending message to server\n");
        exit(1);
    }
    
    

    struct sockaddr_storage sa_stor;
    socklen_t sa_stor_size = sizeof(sa_stor);
    ssize_t bytesReceived;

    int bufLen = 10;
    char buf[bufLen];

    //Receive message from server
    if((bytesReceived = recvfrom(socketFD, buf, bufLen, 0, (struct sockaddr *) &sa_stor, &sa_stor_size)) == -1){
        printf("Error receiving message from server");
        exit(1);
    }
    
    
    //Check return message from server
    if(strcmp(buf, "yes") != 0){
        exit(1);
    }

    printf("A file transfer can start.\n");
    close(socketFD);
     */
    return 0;
}
