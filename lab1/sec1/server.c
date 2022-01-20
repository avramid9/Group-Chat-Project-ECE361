#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int main(int argc, char *argv[]){
    printf("Hello world\n");
    server(argv[0]);
    
    
    return 0;
}

int server(int port_num){
    int socket1 = socket(AF_INET,SOCK_DGRAM,0);
    
    struct sockaddr_storage deliver_addr;  //this will save the ip address of client
    socklen_t addr_len;
    int bytes_sent;
    int bytes_received;
    char buf[5];
    //code used in Beej's Guide to aqcuire IP
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;  // will point to the results

    memset(&hints, 0, sizeof(hints)); // make sure the struct is empty
    hints.ai_family = AF_INET;     // IPv4 
    hints.ai_socktype = SOCK_DGRAM; // UDP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    if ((status = getaddrinfo(NULL, port_num, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }
    //end of code taken from Beej
    
    if(bind(socket1, servinfo->ai_addr, servinfo->ai_addrlen) == -1){
        //fail to bind
        close(socket1);
        printf("failed to bind socket");
        return -1;
    }
    
    freeaddrinfo(servinfo);
    
    //server is set up and listening at port port_num...
    printf("listener: waiting to recvfrom...\n");
    
    addr_len = sizeof(deliver_addr); 
    if((bytes_received = recvfrom(socket1, buf, sizeof(buf) , 0, (struct sockaddr *)&deliver_addr, &addr_len)) == -1){
        //receive failed
        printf("listener:recvfrom failed");
        return -1;
    }
    buf[bytes_received]='\0';
    
    
    //respond to client
    if(buf=="ftp"){
        //return yes to client
        char message [4] = "yes";
        if((bytes_sent = sendto(socket1,message,sizeof(message),0,(struct sockaddr *)&deliver_addr, &addr_len))==-1){
            //response to client failed
            printf("listener:sendto failed");
            return -1;
        }
        
    }
    else{
        //return no to client
        char message [3] = "no";
        if((bytes_sent = sendto(socket1,message,sizeof(message),0,(struct sockaddr *)&deliver_addr, &addr_len))==-1){
            //response to client failed
            printf("listener:sendto failed");
            return -1;
        }
    }
    
    //done
    close(socket1);
    return 0;
}