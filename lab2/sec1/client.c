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
    char buffer[400];
    fd_set readfds;
    int num=0;
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    
            
    while(true){     
        printf("Please login with /login <client ID> <password> <server-IP> <server-port>: ");
        scanf("%s %s %s %s %s",&command,&client_id, &password, &server_id, &server_port); 
        //strcpy(command,"/login");
        if(strcmp(command,"/login")==0){
            //scanf("%s %s %s %s", );
            login_status = login(socketFD, client_id, password, server_id, server_port); //login() for login code
            //login_status=true;
            
            FD_ZERO(&readfds);
            FD_SET(0,&readfds);
            FD_SET(socketFD,&readfds);
            
            printf("command: ");
            while (login_status){
                if(select(1,&readfds,NULL,NULL,NULL)<0){ //set up multiplex with select
                    printf("error setting up select()\n");
                    return 0;
                }               
                
                
                if(FD_ISSET(socketFD,&readfds)){
                    //server sent packet, process
                    printf("server sent stuff\n");

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

                    while(bytesRecv < recvSize){
                        if ((bytesRecv += recv(socketFD, &recvString[bytesRecv], sizeof(recvString) - bytesRecv, 0)) == -1) {
                            printf("Error receiving recv buff message\n");
                            return 0;
                        }
                    }
                    
                    struct message recvMessage = string_to_message(recvString);
                    char* token;
                    switch(recvMessage.type){
                        case JN_ACK:
                            in_sesh = true;
                            printf("%s: joined %s\n",recvMessage.source,recvMessage.data);
                            break;
                        case JN_NAK:
                            in_sesh = false;
                            printf("%s: %s\n",recvMessage.source,recvMessage.data);
                            break;
                        case NS_ACK:
                            printf("created session\n");
                            break;
                        case QU_ACK:
                            printf("%s: %s\n",recvMessage.source,recvMessage.data);
                            break;
                        case MESSAGE:
                            printf("%s: %s\n",recvMessage.source,recvMessage.data);
                        default:
                            printf("Unexpected ack type\n");
                            break;
                    }
                            
                    
                    
                    
                }
                else if(FD_ISSET(0,&readfds)){
                    //check for IO input
                    //need a way to grab keyboard input
                   
                    fgets(buffer,400,stdin);
                                       
                    char *token;
   
                    /* get the first token */
                    token = strtok(buffer, " ");
                    
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
                        
                        token = strtok(NULL," ");
                        if(token==NULL)
                            printf("Missing id\n");
                        else{
                            if(in_sesh)
                                leave_session(socketFD, client_id);
                            create_session(socketFD, client_id, token);
                            join_session(socketFD, client_id, token);
                        }
                    }
                    else if(strcmp(token,"/joinsession")==0){
                        token = strtok(NULL," ");
                        if(token==NULL)
                            printf("Missing id\n");
                        else{
                            if(in_sesh)
                                leave_session(socketFD, client_id);
                            join_session(socketFD, client_id, token);
                        }
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
                    else{ //text
                        if(!in_sesh)
                            printf("please join or create session first.\n");
                        else{                           
                            send_message(socketFD, client_id, buffer);                      
                        }
                    }     
                            
                }
                      
            }
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

    //Connect to server
    int err = connect(socketFD, (const struct sockaddr*)&sa, (socklen_t) sizeof(sa));
    if(err) {
        printf("Connection to server failed.\n");
        return false;
    }

    //Create message
    strcpy(client_id,clientID);
    struct message loginMessage = {.type = LOGIN, .size = strlen(password)};
    strcpy(loginMessage.source, clientID);
    memcpy(loginMessage.data, password, strlen(password));

    //Send message
    char* loginString = message_to_string(loginMessage);
    int bytesSent;
    if ((bytesSent = send(socketFD, loginString, getLenFromString(loginString), 0)) == -1) {
        printf("Failed to send login message.\n");
        return false;
    }

    //Get recv size
    char recvBuff[3];
    int bytesRecv;
    if ((bytesRecv = recv(socketFD, recvBuff, sizeof(recvBuff), 0)) == -1) {
        printf("Error receiving recv buff size\n");
        return false;
    }

    //Get recv message
    int recvSize = getLenFromString(recvBuff)-3;
    //int bytesRecv = 0;
    char recvString[recvSize];
    
    while(bytesRecv < recvSize){
        if ((bytesRecv += recv(socketFD, &recvString[bytesRecv], recvSize - bytesRecv, 0)) == -1) {
            printf("Error receiving recv buff message\n");
            return false;
        }
    }

    //Process ack
    struct message recvMessage = string_to_message(recvString);
    if (recvMessage.type == LO_ACK) {
        printf("Logged in!\n");
        return true;
    }
    else if (recvMessage.type == LO_NAK){
        //for (int i = 0; i < recvMessage.size; i++) {
          //  printf("%c", recvMessage.data[i]);
        //}
        printf("%s\n",recvMessage.data);
        return false;
    }
    else {
        printf("Error getting login ack.\n");
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
    struct message newsessionMessage = {.type = NEW_SESS, .size = strlen(sessionID)};
    strcpy(newsessionMessage.source, clientID);
    memcpy(newsessionMessage.data, sessionID, strlen(sessionID));

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
    struct message joinsessionMessage = {.type = JOIN, .size = strlen(sessionID)};
    strcpy(joinsessionMessage.source, clientID);
    memcpy(joinsessionMessage.data, sessionID, strlen(sessionID));

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
    struct message sendMessage = {.type = MESSAGE, .size = strlen(message)};
    strcpy(sendMessage.source, clientID);
    memcpy(sendMessage.data, message, strlen(message));

    //Send leave message
    char* sendString = message_to_string(sendMessage);
    int bytesSent;
    if ((bytesSent = send(socketFD, sendString, getLenFromString(sendString), 0)) == -1) {
        printf("Failed to send message.\n");
        return;
    }
    
    printf("sent\n");
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
