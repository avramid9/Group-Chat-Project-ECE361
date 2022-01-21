#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

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
    
    //Put server information into sockaddr_in
    struct sockaddr_in sa;
    memset(&sa.sin_zero, 0, 8*sizeof(unsigned char));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress, &(sa.sin_addr));

    ssize_t bytesSent;
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
    return 0;
}