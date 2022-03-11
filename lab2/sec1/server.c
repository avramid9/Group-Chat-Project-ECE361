#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdbool.h>


#define MAX_NAME 100
#define MAX_DATA 1000

//lookup table between id and session_id(in string form)
char sesh_id_to_name[10][MAX_NAME];



struct client_info {
    char id[MAX_NAME];
    char password[25];
    char in_session[MAX_NAME];
    bool login_status;
};

struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

struct client_info client_list[4];

void initialize_client_list();
struct message string_to_message(char* s, int* len);

int main(int argc, char *argv[]) {
    //setup hardcoded client infos
    initialize_client_list();   
    
    //server overall structure taken from Beej Ch.7 select()
    
    fd_set master; // master file descriptor list
    fd_set read_fds; // temp file descriptor list for select()
    int fdmax; // maximum file descriptor number
    
    int status;
    int listener; // listening socket descriptor
    int newfd; // newly accept()ed socket descriptor
    struct sockaddr_storage clientaddr; // client address
    socklen_t addrlen;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    
    
    FD_ZERO(&master); // clear the master and temp sets
    FD_ZERO(&read_fds);
    
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    
    if ((status = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return 0;
    }
    
    if((listener = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol))<0){
        printf("error setting socket");
        return 0;
    }
        
    
    if((bind(listener,servinfo->ai_addr, servinfo->ai_socktype, servinfo->ai_addrlen))<0){
        printf("error binding listener");
        return 0;
    }
        
    
    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one
    
    while(true){
        read_fds = master; // copy it
        if(select(fdmax+1,&read_fds,NULL,NULL,NULL)<0){ //set up multiplex with select
            printf("error setting up select()");
            return 0;
        }
        
        //loop through existing connections
        for(int conns=0; conns<=fdmax; conns++){
            if (FD_ISSET(conns, &read_fds)) { //if int conns is in the set of fd
                if(conns==listener){ //if it's listener, that means someone new is trying to connect
                    addrlen = sizeof(clientaddr);
                    if((newfd = accept(listener,(struct sockaddr *)&clientaddr,addrlen))<0){
                        printf("error accepting client");
                        return 0;
                    }
                    else{
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) { // keep track of the max
                            fdmax = newfd;
                        }
                        printf("new connection");
                    }                   
                }
                else{ //if data not coming from listener, that means clients sent stuff to server
                    
                }
            }
        }
    }
    
    return 0;
}

void initialize_client_list(){
    client_list[0].id = "bob";
    client_list[0].password="123";
    client_list[0].in_session=NULL;
    client_list[0].login_status=false;
    
    client_list[1].id = "john";
    client_list[1].password="123";
    client_list[1].in_session=NULL;
    client_list[1].login_status=false;
    
    client_list[2].id = "steve";
    client_list[2].password="123";
    client_list[2].in_session=NULL;
    client_list[2].login_status=false;
    
    client_list[3].id = "harold";
    client_list[3].password="123";
    client_list[3].in_session=NULL;
    client_list[3].login_status=false;
    
    client_list[4].id = "johnson";
    client_list[4].password="123";
    client_list[4].in_session=NULL;
    client_list[4].login_status=false;
    
    return;
}


struct message string_to_message(char* s, int* len) {
    struct message m;

    //Get len
    *len = (int)((s[1] << 8) + s[0]);

     //Get type
    int start = 3;
    int end = 3;
    char val = s[end];
    while(val != ':'){
        end++;
        val = s[end];
    }
    char type[end-start+1];
    memcpy(type, &s[start], end);
    type[end-start] = '\0';
    m.type = atoi(type);
    end++;
    start = end;

    //Get size
    val = s[end];
    while(val != ':'){
        end++;
        val = s[end];
    }
    char size[end-start+1];
    memcpy(size, &s[start], end);
    size[end-start] = '\0';
    m.size = atoi(size);
    end++;
    start = end;

    //Get source
    val = s[end];
    while(val != ':'){
        end++;
        val = s[end];
    }
    strncpy(m.source, &s[start], end);
    m.source[end-start] = '\0';
    end++;
    start = end;

    //Get data
    memcpy(m.data, &s[start], m.size);

    return m;
}
