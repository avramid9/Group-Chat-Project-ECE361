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
#define PM 14
#define NEW 15
#define NEW_ACK 16
#define NEW_NAK 17
#define PM_NAK 18


#define MAX_NAME 100
#define MAX_DATA 1000

struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

bool login(int socketFD, char* clientID, char* password, char* serverIP, char* serverPort);
void getList(int socketFD, char* clientID);
bool logout(int socketFD, char* clientID);
void create_session(int socketFD, char* clientID, char* sessionID);
void join_session(int socketFD, char* clientID, char* sessionID);
bool leave_session(int socketFD, char* clientID);
void send_message(int socketFD, char* clientID, char* message);
void send_pm(int socketFD, char* clientID, char* recipiantID, char* message);
void create(int socketFD, char* clientID, char* password, char* serverIP, char* serverPort);
char* message_to_string(struct message m);
struct message string_to_message(char* s);
int getLenFromString(char* s);



char client_id[20];
int main() {
    char sesh_id[30];
    char command[20];   
    char password[20];
    char server_id[20];
    char server_port[20];
    char session_list[20][20];
    char text[200];
    bool login_status=false;
    bool in_sesh = false;
    
    fd_set readfds;
    int num=0;
    
    
            
    while(true){  
        int socketFD = socket(AF_INET, SOCK_STREAM, 0);
        printf("Please login with /login <client ID> <password> <server-IP> <server-port>\n");
        printf("or create account with /create <client ID> <password> <server-IP> <server-port>\n");
        scanf("%s",command); 
        //strcpy(command,"/login");
        if(strcmp("/login",command)==0){
            scanf(" %s %s %s %s",client_id, password, server_id, server_port);            
            login_status = login(socketFD, client_id, password, server_id, server_port); //login() for login code
            
            while (login_status){
                //printf("before select\n");
                FD_ZERO(&readfds);
                FD_SET(0,&readfds);
                FD_SET(socketFD,&readfds);
                
                if(select(socketFD+1,&readfds,NULL,NULL,NULL)<0){ //set up multiplex with select
                    printf("error setting up select()\n");
                    return 0;
                }               
                //printf("after select\n");
                if(FD_ISSET(0,&readfds)){
                        //check for IO input
                        //need a way to grab keyboard input
                        //printf("key pressed\n");
                        char token[MAX_DATA];
                        scanf("%s",token);
                        
                        if(strcmp(token,"/logout")==0){
                            if(in_sesh)
                                in_sesh = leave_session(socketFD, client_id);
                            login_status = logout(socketFD, client_id);//code to close connection if there is one
                            printf("Logging out\n");
                        }
                        else if(strcmp(token,"/list")==0){
                            getList(socketFD, client_id);
                        }
                        else if(strcmp(token,"/createsession")==0){
                            char arg[100];
                            scanf(" %s",arg);
                            
                            if(in_sesh)
                                leave_session(socketFD, client_id);
                            create_session(socketFD, client_id, arg);
                            join_session(socketFD, client_id, arg);
                           
                        }
                        else if(strcmp(token,"/joinsession")==0){
                            char arg[100];
                            scanf(" %s",arg);
                            if(in_sesh)
                                leave_session(socketFD, client_id);
                            join_session(socketFD, client_id, arg);
                            
                        }
                        else if(strcmp(token,"/leavesession")==0){
                            if(!in_sesh)
                                printf("please join or create session first.\n");
                            else
                                in_sesh = leave_session(socketFD, client_id);
                        }               
                        else if(strcmp(token,"/quit")==0){
                            if(in_sesh)
                                in_sesh = leave_session(socketFD, client_id);
                            login_status = logout(socketFD, client_id);
                            printf("Quitting client.\n");
                            return 0;
                        }
                        else if(strcmp(token, "/pm")==0){
                            char arg1[100];
                            char arg2[MAX_DATA];
                            scanf(" %s ", arg1);
                            fgets(arg2,MAX_DATA,stdin);
                            send_pm(socketFD, client_id, arg1, arg2);
                        }
                        else if(strcmp(token,"/create")==0){
                            char arg[100];
                            char arg2[100];
                            scanf(" %s %s",arg,arg2);
                            printf("Please logout first.\n");
                        }
                        else{ //text
                            if(!in_sesh)
                                printf("please join or create session first.\n");
                            else{                    
                                //need to change back
                                char msg[MAX_DATA];
                                strcpy(msg,token);
                                int offset = strlen(token);
                                fgets(msg+offset,MAX_DATA-offset,stdin);
                                send_message(socketFD, client_id, msg);                      
                            }
                        }
                        
                }
                if(FD_ISSET(socketFD,&readfds)){                       
                        //server sent packet, process
                        //printf("server sent stuff\n");

                        //Get recv size
                        char recvBuff[3];
                        int bytesRecv;
                        if ((bytesRecv = recv(socketFD, recvBuff, sizeof(recvBuff), 0)) == -1) {
                            printf("Error receiving recv buff size\n");
                            return 0;
                        }

                        //Get recv message
                        int recvSize = getLenFromString(recvBuff)-3;
                        //int bytesRecv = 0;
                        char recvString[recvSize];
                        bytesRecv=0;
                        while(bytesRecv < recvSize){
                            if ((bytesRecv += recv(socketFD, recvString+bytesRecv, recvSize - bytesRecv, 0)) == -1) {
                                printf("Error receiving recv buff message\n");
                                return 0;
                            }
                        }
                        
                        struct message recvMessage = string_to_message(recvString);
                        //printf("test:%u %u %s %s\n",recvMessage.type,recvMessage.size,recvMessage.source,recvMessage.data);
                        char* token;
                        switch(recvMessage.type){
                            case JN_ACK:
                                in_sesh = true;
                                printf("%s: you joined %s\n",recvMessage.source,recvMessage.data);
                                break;
                            case JN_NAK:
                                in_sesh = false;
                                printf("%s: %s\n",recvMessage.source,recvMessage.data);
                                break;
                            case NS_ACK:
                                printf("%s: you created session\n",recvMessage.source);
                                break;
                            case QU_ACK:
                                printf("%s: %s\n",recvMessage.source,recvMessage.data);
                                break;
                            case MESSAGE:
                                printf("%s: %s\n",recvMessage.source,recvMessage.data);
                                break;
                            case PM_NAK:
                                printf("%s\n", recvMessage.data);
                                break;
                            default:
                                printf("Unexpected ack type\n");
                                break;
                        }
                        
                }   
                
                    
                
                      
            }
        }
        else if(strcmp(command,"/create")==0){
            scanf(" %s %s %s %s",client_id, password, server_id, server_port);
            int tempFD = socket(AF_INET, SOCK_STREAM, 0);
            create(tempFD, client_id, password, server_id, server_port);
            
        }
        else if(strcmp(command,"/quit")==0){
            printf("Quitting client.\n");
            return 0;
        }
        else
            printf("Invalid command.\n");
    }   
    
    return 0;
}

bool login(int socketFD, char* clientID, char* password, char* serverIP, char* serverPort){
    //Put server information into sockaddr_in
    struct sockaddr_in sa;
    memset(&sa.sin_zero, 0, 8*sizeof(unsigned char));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(atoi(serverPort));
    inet_pton(AF_INET, serverIP, &(sa.sin_addr));
    //printf("attempting login\n");
    //fflush(stdout);
    //Connect to server
    int err = connect(socketFD, (const struct sockaddr*)&sa, (socklen_t) sizeof(sa));
    if(err) {
        printf("Connection to server failed.\n");
        return false;
    }
    //printf("connected\n");
    //fflush(stdout);
    //Create message
    strcpy(client_id,clientID);
    struct message loginMessage = {.type = LOGIN, .size = strlen(password)+1};
    strcpy(loginMessage.source, clientID);
    strcpy(loginMessage.data, password);
    
    
    //Send message
    char* loginString = message_to_string(loginMessage);
    int bytesSent;
    
    
    if ((bytesSent = send(socketFD, loginString, getLenFromString(loginString), 0)) == -1) {
        printf("Failed to send login message.\n");
        fflush(stdout);
        return false;
    }

    //Get recv size
    char recvBuff[3];
    int bytesRecv;
    if ((bytesRecv = recv(socketFD, recvBuff, sizeof(recvBuff), 0)) == -1) {
        printf("Error receiving recv buff size\n");
        fflush(stdout);
        return false;
    }

    //Get recv message
    int recvSize = getLenFromString(recvBuff)-3;
    //int bytesRecv = 0;
    char recvString[recvSize];
    bytesRecv=0;
    while(bytesRecv < recvSize){
        if ((bytesRecv += recv(socketFD, recvString+bytesRecv, recvSize - bytesRecv, 0)) == -1) {
            printf("Error receiving recv buff message\n");
            fflush(stdout);
            return false;
        }
    }

    //Process ack
    struct message recvMessage = string_to_message(recvString);
    if (recvMessage.type == LO_ACK) {
        printf("Logged in!\n");
        fflush(stdout);
        return true;
    }
    else if (recvMessage.type == LO_NAK){
        //for (int i = 0; i < recvMessage.size; i++) {
          //  printf("%c", recvMessage.data[i]);
        //}
        printf("%s\n",recvMessage.data);
        fflush(stdout);
        return false;
    }
    else {
        printf("Error getting login ack.\n");
        fflush(stdout);
    }

    return false;
}

bool logout(int socketFD, char* clientID){
    //Create logout message
    struct message logoutMessage = {.type = EXIT, .size = 0};
    strcpy(logoutMessage.source, clientID);

    //Send message
    char* logoutString = message_to_string(logoutMessage);
    int bytesSent;
    if ((bytesSent = send(socketFD, logoutString, getLenFromString(logoutString), 0)) == -1) {
        printf("Failed to send logout message.\n");
        return true;
    }

    //Not sure if close() should be called here or on server
    close(socketFD);

    printf("goodbye\n");
    return false;
}

void getList(int socketFD, char* clientID){
    //Create list message
    struct message listMessage = {.type = QUERY, .size = 0};
    strcpy(listMessage.source, clientID);
    
    //Send list message
    char* listString = message_to_string(listMessage);
    int bytesSent;
    if ((bytesSent = send(socketFD, listString, getLenFromString(listString), 0)) == -1) {
        printf("Failed to send list message.\n");
        return;
    }
    return;
}

void create_session(int socketFD, char* clientID, char* sessionID){
    //Create new session message
    struct message newsessionMessage = {.type = NEW_SESS, .size = strlen(sessionID)+1};
    strcpy(newsessionMessage.source, clientID);
    strcpy(newsessionMessage.data, sessionID);

    //Send create session message
    char* newsessionString = message_to_string(newsessionMessage);
    int bytesSent;
    if ((bytesSent = send(socketFD, newsessionString, getLenFromString(newsessionString), 0)) == -1) {
        printf("Failed to send new session message.\n");
        return;
    }

    
    return;
}

void join_session(int socketFD, char* clientID, char* sessionID){
    //Create join session message
    struct message joinsessionMessage = {.type = JOIN, .size = strlen(sessionID)+1};
    strcpy(joinsessionMessage.source, clientID);
    strcpy(joinsessionMessage.data, sessionID);

    //Send join message
    char* joinsessionString = message_to_string(joinsessionMessage);
    int bytesSent;
    if ((bytesSent = send(socketFD, joinsessionString, getLenFromString(joinsessionString), 0)) == -1) {
        printf("Failed to send new session message.\n");
        return;
    }


    
    return;
}

bool leave_session(int socketFD, char* clientID){
    //Create join session message
    struct message leavesessionMessage = {.type = LEAVE_SESS, .size = 0};
    strcpy(leavesessionMessage.source, clientID);

    //Send leave message
    char* leavesessionString = message_to_string(leavesessionMessage);
    int bytesSent;
    if ((bytesSent = send(socketFD, leavesessionString, getLenFromString(leavesessionString), 0)) == -1) {
        printf("Failed to send new session message.\n");
        return false;
    }
    
    printf("left session\n");
    return false;
}

void send_message(int socketFD, char* clientID, char* message){
    //Create join session message
    struct message sendMessage = {.type = MESSAGE, .size = strlen(message)+1};
    strcpy(sendMessage.source, clientID);
    strcpy(sendMessage.data, message);

    //Send message
    char* sendString = message_to_string(sendMessage);
    int bytesSent;
    if ((bytesSent = send(socketFD, sendString, getLenFromString(sendString), 0)) == -1) {
        printf("Failed to send message.\n");
        return;
    }
    
    //printf("sent\n");
    return;
}

void send_pm(int socketFD, char* clientID, char* recipiantID, char* message){
    //Create pm message
    struct message sendMessage = {.type = PM, .size = strlen(message)+strlen(recipiantID)+2};
    strcpy(sendMessage.source, clientID);
    strcat(sendMessage.data, recipiantID);
    strcat(sendMessage.data, " ");
    strcat(sendMessage.data, message);
    
    //Send pm
    char* sendString = message_to_string(sendMessage);
    int bytesSent;
    if ((bytesSent = send(socketFD, sendString, getLenFromString(sendString), 0)) == -1) {
        printf("Failed to send message.\n");
        return;
    }
    
    return;
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

    //Get len
    //*len = (int)((s[1] << 8) + s[0]);

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

void create(int socketFD, char* clientID, char* password, char* serverIP, char* serverPort){
    //Put server information into sockaddr_in
    struct sockaddr_in sa;
    memset(&sa.sin_zero, 0, 8*sizeof(unsigned char));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(atoi(serverPort));
    inet_pton(AF_INET, serverIP, &(sa.sin_addr));
    
    int err = connect(socketFD, (const struct sockaddr*)&sa, (socklen_t) sizeof(sa));
    if(err) {
        printf("Connection to server failed.\n");
        return;
    }
    //printf("connected\n");
    //fflush(stdout);
    //Create message
    strcpy(client_id,clientID);
    struct message loginMessage = {.type = NEW, .size = strlen(password)+1};
    strcpy(loginMessage.source, clientID);
    strcpy(loginMessage.data, password);
    
    
    //Send message
    char* loginString = message_to_string(loginMessage);
    int bytesSent;
    
    
    if ((bytesSent = send(socketFD, loginString, getLenFromString(loginString), 0)) == -1) {
        printf("Failed to send login message.\n");
        fflush(stdout);
        return;
    }

    //Get recv size
    char recvBuff[3];
    int bytesRecv;
    if ((bytesRecv = recv(socketFD, recvBuff, sizeof(recvBuff), 0)) == -1) {
        printf("Error receiving recv buff size\n");
        fflush(stdout);
        return;
    }

    //Get recv message
    int recvSize = getLenFromString(recvBuff)-3;
    //int bytesRecv = 0;
    char recvString[recvSize];
    bytesRecv=0;
    while(bytesRecv < recvSize){
        if ((bytesRecv += recv(socketFD, recvString+bytesRecv, recvSize - bytesRecv, 0)) == -1) {
            printf("Error receiving recv buff message\n");
            fflush(stdout);
            return;
        }
    }

    //Process ack
    struct message recvMessage = string_to_message(recvString);
    if (recvMessage.type == NEW_ACK) {
        printf("Account created!\n");       
    }
    else if (recvMessage.type == NEW_NAK){
        printf("%s\n",recvMessage.data);
    }
    else {
        printf("Error getting create ack.\n");
    }
    return;
}