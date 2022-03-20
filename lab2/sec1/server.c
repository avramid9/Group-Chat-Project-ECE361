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
#include <arpa/inet.h>

#define LOGIN 1
#define LO_ACK 2
#define LO_NAK 3
#define EXIT 4
#define JOIN 5
#define JN_ACK 6
#define JN_NAK 7
#define LEAVE_SESS 8
#define NEW_SESS 9
#define NS_ACK 10
#define MESSAGE 11
#define QUERY 12
#define QU_ACK 13

#define MAX_NAME 100
#define MAX_DATA 1000

//lookup table between id and session_id(in string form)
char sesh_id_to_name[20][MAX_NAME];



struct client_info {
    char id[MAX_NAME];
    char password[25];
    char in_session[MAX_NAME];
    bool login_status;
    int fd;
    char ip[INET_ADDRSTRLEN];
    int port;
};

struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

struct client_info client_list[5];
int list_size = 5;
int sesh_list_size = 20;

void initialize_client_list();

char* message_to_string(struct message m);
struct message string_to_message(char* s);
int getLenFromString(char* s);
int client_id_from_name(char* s);
struct message string_to_message(char* s);
void send_login_ack(int conns, int id, struct message* m);
void send_join_ack(int conns, int id, struct message* m);
void leave_sesh(int conns, int id, struct message* m);
void new_sesh(int conns, int id, struct message* m);
void send_join_ack(int conns, int id, struct message* m);
void send_list(int conns, int id, struct message* m);
void message_ppl(int conns, int id, struct message* m);


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
    int bytes_rec;
    
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
        printf("error setting socket\n");
        return 0;
    }
        
    
    if((bind(listener,servinfo->ai_addr, servinfo->ai_addrlen))<0){
        printf("error binding listener\n");
        return 0;
    }
    
    if(listen(listener, 10)==-1){
        printf("error listening\n");
        return 0;
    }
    
    // add the listener to the master set
    FD_SET(listener, &master);
    printf("listening for clients\n");
    //fflush(stdout);
    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one
    
    while(true){
        read_fds = master; // copy it
        if(select(fdmax+1,&read_fds,NULL,NULL,NULL)<0){ //set up multiplex with select
            printf("error setting up select()\n");
            return 0;
        }
        char message_size [3];
        
        //loop through existing connections
        for(int conns=0; conns<=fdmax; conns++){
            if(FD_ISSET(conns, &read_fds)) { //if int conns is in the set of fd
                if(conns==listener){ //if it's listener, that means someone new is trying to connect
                    addrlen = sizeof(clientaddr);
                    if((newfd = accept(listener,(struct sockaddr *)&clientaddr,&addrlen))<0){
                        printf("error accepting client %d\n",newfd);
                        return 0;
                    }
                    
                    FD_SET(newfd, &master); // add to master set
                    if (newfd > fdmax) { // keep track of the max
                        fdmax = newfd;
                    }
                    printf("new connection\n");
                    //fflush(stdout);
                                       
                }
                else{ //if data not coming from listener, that means clients sent stuff to server
                    //printf("received packet!\n");
                    //fflush(stdout);
                    if((bytes_rec = recv(conns,message_size,3,0))!=-1){
                        //printf("in first while\n");
                        if(bytes_rec==0)
                            printf("connection closed\n");
                        //Get recv message
                        int recvSize = getLenFromString(message_size)-3;
                        printf("%d\n",recvSize);
                        //int bytesRecv = 0;
                        char recvString[recvSize];
                        bytes_rec=0;
                        while(bytes_rec < recvSize){
                            //printf("received: %d\n",bytes_rec);
                            if ((bytes_rec += recv(conns, recvString+bytes_rec, recvSize - bytes_rec, 0)) == -1) {
                                printf("Error receiving recv buff message\n");
                                return 0;
                            }
                            //printf("received after: %d\n",bytes_rec);
                        }
                        //for(int i=0;i<recvSize;i++){
                        //    printf("%c",recvString[i]);

                        //}
                        //printf("\n");
                        //convert serial to message
                        struct message m = string_to_message(recvString);
                        // first check if they're logged in
                        //printf("%u %u %s %s\n",m.type, m.size, m.source, m.data);
                        int id = client_id_from_name(m.source);
                        if(m.type==LOGIN)
                            send_login_ack(conns,id,&m);
                        else if(m.type==JOIN)
                            send_join_ack(conns,id,&m);
                        else if(m.type==LEAVE_SESS)
                            leave_sesh(conns,id,&m);
                        else if(m.type==NEW_SESS)
                            new_sesh(conns,id,&m);
                        else if(m.type==QUERY)
                            send_list(conns,id,&m);
                        else if(m.type==MESSAGE)
                            message_ppl(conns,id,&m);
                        else if(m.type==EXIT){
                            //logout
                            client_list[id].login_status=false;
                            close(client_list[id].fd);
                            FD_CLR(client_list[id].fd,&master);
                            client_list[id].fd=-1;
                            client_list[id].port=-1;
                            memset(&client_list[id].ip[0],0,sizeof(client_list[id].ip));
                        }
                            
                    }
                }
            }
        }
    }
    
    return 0;
}

void initialize_client_list(){
    
    strcpy(client_list[0].id,"bob");
    strcpy(client_list[0].password,"123");
    strcpy(client_list[0].in_session,"none");
    client_list[0].login_status=false;
    client_list[0].fd=-1;
    
    strcpy(client_list[1].id,"john");
    strcpy(client_list[1].password,"123");
    strcpy(client_list[1].in_session,"none");
    client_list[1].login_status=false;
    client_list[1].fd=-1;
    
    strcpy(client_list[2].id,"steve");
    strcpy(client_list[2].password,"123");
    strcpy(client_list[2].in_session,"none");
    client_list[2].login_status=false;
    client_list[2].fd=-1;
    
    strcpy(client_list[3].id,"harold");
    strcpy(client_list[3].password,"123");
    strcpy(client_list[3].in_session,"none");
    client_list[3].login_status=false;
    client_list[3].fd=-1;
    
    strcpy(client_list[4].id,"johnson");
    strcpy(client_list[4].password,"123");
    strcpy(client_list[4].in_session,"none");
    client_list[4].login_status=false;
    client_list[4].fd=-1;
    return;
}


bool last_in_sesh(char* s){
    for(int i=0;i<list_size;i++){
        if(strcmp(client_list[i].in_session,s)==0)
            return false;
    }
    return true;
}


int client_id_from_name(char* s){
    for(int i=0;i<list_size;i++){
        if(strcmp(client_list[i].id,s)==0)
            return i;
    }
    return -1;
}


char* message_to_string(struct message m) {
    //Get strings and their length from first 2 fields in packet
    int type_len = snprintf(NULL, 0, "%d", m.type);
    char* type = malloc(type_len + 1);
    snprintf(type, type_len+1, "%d", m.type);

    int size_len = snprintf(NULL, 0, "%d", m.size);
    char* size = malloc(size_len + 1);
    snprintf(size, size_len+1, "%d", m.size);

    //Allocate char array for the message which will be returned as a pointer
    int sSize = 6 + type_len + size_len + strlen(m.source) + m.size;
    char *s = malloc(sSize);
    
    //Keep track of position each time we add a string to the message
    int sPos = 0;

    //Add size bytes at begining
    s[0] = (char) (sSize & 0xff);
    s[1] = (char) ((sSize >> 8) & 0xff);
    s[2] =  ':';
    sPos = 3;

    //Add first two variables to message
    for(int i = sPos; i < (sPos + type_len); i++){
        s[i] = type[i-sPos];
    }
    s[sPos + type_len] = ':';
    sPos += type_len+1;
    
    for(int i = sPos; i < (sPos + size_len); i++){
        s[i] = size[i-sPos];
    }
    s[sPos + size_len] = ':';
    sPos += size_len+1;
    
    //Add source to message
    for(int i = sPos; i <= sPos + strlen(m.source); i++){
        s[i] = m.source[i-sPos];
    }
    s[sPos + strlen(m.source)] = ':';
    sPos += strlen(m.source)+1;
    
    //Add data to message
    for(int i = sPos; i < sPos + m.size; i++){
        s[i] = m.data[i-sPos];
    }

    free(type);
    free(size);
    
    return s;
}

struct message string_to_message(char* s) {
    struct message m;
    
     //Get type
    int start = 0;
    int end = 0;
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


int getLenFromString(char* s) {
    return (int)((s[1] << 8) + s[0]);
}

void send_login_ack(int conns, int id, struct message* m){
    //printf("respond login\n");
    struct message response;
    strcpy(response.source,"server");
    if(client_list[id].login_status){
        response.type=LO_NAK;
        strcpy(response.data,"User already logged in");
        response.size = strlen(response.data)+1;
    }
    else if(id==-1){
        response.type=LO_NAK;
        strcpy(response.data,"User not found");
        response.size = strlen(response.data)+1;
    }
    else if(strcmp(m->data,client_list[id].password)!=0){
        response.type=LO_NAK;
        strcpy(response.data,"Wrong password");
        response.size = strlen(response.data)+1;
    }
    else{
        socklen_t len;
        struct sockaddr_storage addr;
        len = sizeof addr;
        getpeername(conns, (struct sockaddr*)&addr, &len);
        
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        client_list[id].port = ntohs(s->sin_port);
        inet_ntop(AF_INET, &s->sin_addr, client_list[id].ip, sizeof client_list[id].ip);
        
        printf("%s logged in from ip: %s, port: %d\n",m->source,client_list[id].ip,client_list[id].port);
        response.type=LO_ACK;
        response.size=0;
        
        client_list[id].login_status=true;
        client_list[id].fd = conns;
    }
    char* p = message_to_string(response);
    if(send(conns,p,getLenFromString(p),0)==-1)
        printf("error sending login ack\n");
    return;
}

void send_join_ack(int conns, int id, struct message* m){
    struct message response;
    strcpy(response.source,"server");
    int i=0;       
    for(int i=0;i<sesh_list_size;i++){
        if(strcmp(sesh_id_to_name[i],m->data)==0){
            response.type=JN_ACK;
            response.size=strlen(m->data)+1;
            strcpy(client_list[id].in_session,m->data);
            strcpy(response.data,m->data);
            
            char* p = message_to_string(response);
            int bytesSent;
            if((bytesSent=send(conns,p,getLenFromString(p),0))==-1)
                printf("error sending join ack\n");
            printf("%s joined %s\n",client_list[id].id,m->data);
            return; 
        }
            
    }
    
    response.type=JN_NAK;
    strcpy(response.data,"Session does not exist");
    response.size = strlen("Session does not exist")+1;
    
    char* p = message_to_string(response);
    int bytesSent;
    if((bytesSent = send(conns,p,getLenFromString(p),0))==-1)
        printf("error sending join ack\n");
    printf("%s failed to join %s\n",client_list[id].id,m->data);
    return; 
}
void leave_sesh(int conns, int id, struct message* m){
      
    for(int i=0;i<sesh_list_size;i++){
        if(strcmp(sesh_id_to_name[i],client_list[id].in_session)==0){
            printf("%s left %s\n",m->source,client_list[id].in_session);
            strcpy(client_list[id].in_session,"none");

            //if no one left delete sesh
            bool last=true;
            for(int j=0;j<list_size && last;j++){
                if(strcmp(client_list[j].in_session,sesh_id_to_name[i])==0)
                    last=false;
            }
            if(last)
                strcpy(sesh_id_to_name[i],"");
            return;
        }
           
    }
    return; 
}
void new_sesh(int conns, int id, struct message* m){
    struct message response;
    strcpy(response.source,"server");
    
    for(int i=0;i<sesh_list_size;i++){
        if(strcmp(sesh_id_to_name[i],m->data)==0){
            //session exists
            response.type = NS_ACK;           
            response.size=0;
            char* p = message_to_string(response);
            int bytesSent;
            if((bytesSent = send(conns,p,getLenFromString(p),0))==-1)
                printf("error sending new sesh ack\n");
            printf("session %s exists\n",m->data);
            return;
        }
        if(strcmp(sesh_id_to_name[i],"")==0){
            strcpy(sesh_id_to_name[i],m->data);
            response.type = NS_ACK;           
            response.size=0;
            char* p = message_to_string(response);
            int bytesSent;
            if((bytesSent = send(conns,p,getLenFromString(p),0))==-1)
                printf("error sending new sesh ack\n");
            printf("%s created session %s\n",client_list[id].id,m->data);
            return;
        }
    }
    return;
}

void send_list(int conns, int id, struct message* m){
    struct message response;
    strcpy(response.source,"server");
    response.type=QU_ACK;
    char list[MAX_DATA] = "\nAvailable sessions:\n";
    
    for(int i=0;i<sesh_list_size;i++){
        if(strcmp(sesh_id_to_name[i],"")!=0){
            strcat(list,sesh_id_to_name[i]);
            strcat(list,"\n");
        }
    }
    strcat(list,"\nUsers: \n");
    for(int i=0;i<list_size;i++){
        if(client_list[i].fd!=-1){
           strcat(list,client_list[i].id);
            strcat(list," --> ");
            if(strcmp(client_list[i].in_session,"none")==0)
                strcat(list,"none");
            else
                strcat(list,client_list[i].in_session);
            //if(i!=list_size-1)
            strcat(list,"\n"); 
        }      
    }
    strcpy(response.data,list);
    response.size = strlen(response.data)+1;
    //printf("%u %u %s %s\n",response.type,response.size,response.source,response.data);
    char* p = message_to_string(response);
    int bytesSent;
    if((bytesSent = send(conns,p,getLenFromString(p),0))==-1)
        printf("error sending new sesh ack\n");
    printf("Sent list to %s\n",client_list[id].id);
    return;
}
void message_ppl(int conns, int id, struct message* m){
    char* p = message_to_string(*m);
    printf("%s messaged everyone in session %s\n",client_list[id].id,client_list[id].in_session);
    for(int i=0;i<list_size;i++){
        if(i!=id && strcmp(client_list[i].in_session,client_list[id].in_session)==0){
            int bytesSent;
            if((bytesSent = send(client_list[i].fd,p,getLenFromString(p),0)) == -1)
                printf("error sending message from %s to %s, in %s\n",m->source,client_list[id].id,client_list[id].in_session);
        }
    }
}

