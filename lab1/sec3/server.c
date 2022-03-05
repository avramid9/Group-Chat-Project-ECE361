#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
int server(char *port_num);

struct packet {  
    unsigned int total_frag;  
    unsigned int frag_no; 
    unsigned int size; 
    char* filename; 
    char filedata[1000];  
};

struct packet message_to_packet(char* m){
    struct packet p;
    
    //Get total_frag
    int start = 0;
    int end = 0;
    char val = m[end];
    while(val != ':'){
        end++;
        val = m[end];
    }
    char total_frag[end-start+1];
    memcpy(total_frag, &m[start], end);
    total_frag[end-start] = '\0';
    p.total_frag = atoi(total_frag);
    end++;
    start = end;
    
    //Get frag_no
    val = m[end];
    while(val != ':'){
        end++;
        val = m[end];
    }
    char frag_no[end-start+1];
    memcpy(frag_no, &m[start], end);
    frag_no[end-start] = '\0';
    p.frag_no = atoi(frag_no);
    end++;
    start = end;
    
    //Get size
    val = m[end];
    while(val != ':'){
        end++;
        val = m[end];
    }
    char size[end-start+1];
    memcpy(size, &m[start], end);
    size[end-start] = '\0';
    p.size = atoi(size);
    end++;
    start = end;
    
    //Get filename
    val = m[end];
    while(val != ':'){
        end++;
        val = m[end];
    }
    p.filename = malloc(end-start+1);
    strncpy(p.filename, &m[start], end);
    p.filename[end-start] = '\0';
    end++;
    start = end;
    
    //Get filedata
    memcpy(p.filedata, &m[start], p.size);
    
    return p;
}

int main(int argc, char *argv[]){
    
    server(argv[1]);
    
    return 0;
}

int server(char *port_num){
    int socket1 = socket(AF_INET,SOCK_DGRAM,0);
    
    struct sockaddr_storage deliver_addr;  //this will save the ip address of client
    socklen_t addr_len;
    int bytes_sent;
    int bytes_received;
    char buf[4000];
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
        //close(socket1);
        printf("failed to bind socket");
        return -1;
    }
    
    //server is set up and listening at port port_num...
    printf("listener: waiting to recvfrom...\n");
    
    //////////////////////////////////////////////////
    //listening for RTT packet
    addr_len = sizeof(deliver_addr); 
    if((bytes_received = recvfrom(socket1, buf, sizeof(buf), 0, (struct sockaddr *)&deliver_addr, &addr_len)) == -1){
        //receive failed
        printf("first listener:recvfrom failed");
        return -1;
    }
    else{
        if((bytes_sent = sendto(socket1,"first",sizeof("first"),0,(struct sockaddr *)&deliver_addr, addr_len))==-1){
            //response to client failed
            printf("listener:sendto failed");
            return -1;
        }
    }
    /////////////////////////////////////////////////////////
    
    //begin file transfer
    if((bytes_received = recvfrom(socket1, buf, sizeof(buf), 0, (struct sockaddr *)&deliver_addr, &addr_len)) == -1){
        //receive failed
        printf("first listener:recvfrom failed");
        return -1;
    }
    
    
    struct packet first_packet = message_to_packet(buf);
    
    int fd;
    
    
    
    char path[100]="received/";
    strcat(path,first_packet.filename);
    
    FILE *fp = fopen(path,"ab");
    
    
    size_t writer;
    if((writer=fwrite(first_packet.filedata,1,first_packet.size,fp))<0)
        printf("error writing file");
    
    char ack [10] = "yes";
    if((bytes_sent = sendto(socket1,ack,sizeof(ack),0,(struct sockaddr *)&deliver_addr, addr_len))==-1){
        //response to client failed
        printf("listener:sendto failed");
        return -1;
    }
    
    for(int i=2; i<=first_packet.total_frag; i++){
        if((bytes_received = recvfrom(socket1, buf, sizeof(buf), 0, (struct sockaddr *)&deliver_addr, &addr_len)) == -1){
                //receive failed
                printf("listener:recvfrom failed");
                return -1;
            }
        
        if (uniform_rand() > 1e-2) {
            struct packet p = message_to_packet(buf);
            if((writer=fwrite(p.filedata,1,p.size,fp))<0)
                printf("error writing file");
        
        
            if((bytes_sent = sendto(socket1,ack,sizeof(ack),0,(struct sockaddr *)&deliver_addr, addr_len))==-1){
                //response to client failed
                printf("listener:sendto failed");
                return -1;
            }
        }
        else {
            i--;
        }
    }
    fclose(fp);
    
    
    //respond to client
    char message_return [10] = "completed";
    if((bytes_sent = sendto(socket1,message_return,sizeof(message_return),0,(struct sockaddr *)&deliver_addr, addr_len))==-1){
        //response to client failed
        printf("listener:sendto failed");
        return -1;
    }
    
    //done
    close(socket1);
    return 0;
}