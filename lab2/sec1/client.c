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
#define L_ACK 14
#define L_NAK 15

#define MAX_NAME 100
#define MAX_DATA 1000

struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

bool login();
void getList();
bool logout(int socketFD, char* clientID, char* password, char* serverIP, char* serverPort);
void create_session(char* id);
bool join_session(char* id);
bool leave_session();
void send_message(char* message);
char* message_to_string(struct message m);
struct message string_to_message(char* s);
int getLenFromString(char* s);


char sesh_id[30];

int main() {
    char command[20];
    char arg1[20];
    char arg2[20];
    char arg3[20];
    char arg4[20];
    char session_list[20][20];
    char text[200];
    bool login_status=false;
    bool in_sesh = false;

    int socketFD = socket(AF_INET, SOCK_STREAM, 0);

    while(true){     
        printf("Please login with /login <client ID> <password> <server-IP> <server-port>: ");
        scanf("%s",&command);   
        if(strcmp(command,"/login")==0){
            scanf("%s %s %s %s",&arg1,&arg2,&arg3,&arg4);
            login_status = login(socketFD, arg1, arg2, arg3, arg4); //login() for login code
            while (login_status){
                printf("command:");
                scanf("%s",&command);
                
                if(strcmp(command,"/logout")==0){
                    if(in_sesh)
                        in_sesh = leave_session();
                    login_status = logout();//code to close connection if there is one
                    printf("Logging out\n");
                }
                else if(strcmp(command,"/list")==0){
                    getList();
                }
                else if(strcmp(command,"/createsession")==0){
                    scanf("%s",&arg1);
                    if(in_sesh)
                        leave_session();
                    create_session(arg1);
                    in_sesh = join_session(arg1);
                    
                }
                else if(strcmp(command,"/joinsession")==0){
                    scanf("%s",&arg1);
                    if(in_sesh)
                        printf("please leave session first.\n");
                    else{                               
                        in_sesh = join_session(arg1);
                    }
                }
                else if(strcmp(command,"/leavesession")==0){
                    if(!in_sesh)
                        printf("please join or create session first.\n");
                    else
                        in_sesh = leave_session();
                }               
                else if(strcmp(command,"/quit")==0){
                    login_status = logout();
                    printf("Quitting client.\n");
                    return 0;
                }
                else{ //text
                    if(in_sesh)
                        printf("please join or create session first.\n");
                    else{
                        scanf("%s",&text);
                        send_message(text);                      
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

    int err = connect(socketFD, (const struct sockaddr*)&sa, (socklen_t) sizeof(sa));
    if(err) {
        printf("Connection to server failed.\n");
        return false;
    }

    struct message loginMessage = {.type = LOGIN, .size = sizeof(password), .source = clientID, .data = password};
    char* loginString = message_to_string(loginMessage);
    if ((int bytesSent = send(socketFD, loginString, getLenFromString(loginString), 0)) == -1) {
        printf("Failed to send login message.\n");
        return false;
    }

    char* recvBuff[3];
    if ((int bytesRecv = recv(socketFD, recvBuff, sizeof(recvBuff), 0)) == -1) {
        printf("Error receiving recv buff size\n");
        return false;
    }

    int recvSize = getLenFromString(recvBuff);
    int bytesRecv = 0;
    char* recvString[recvSize];

    while(bytesRecv < recvSize){
        if ((bytesRecv += recv(socketFD, &recvString[bytesRecv], sizeof(recvString) - bytesRecv, 0)) == -1) {
            printf("Error receiving recv buff message\n");
            return false;
        }
    }

    struct messsage recvMessage = string_to_message(recvString);
    if (recvMessage.type == LO_ACK) {
        return true;
    }
    else if (recvMessage.type == LO_NAK){
        for (int i = 0; i < recvMessage.size; i++) {
            printf("%c", recvMessage.data[i]);
        }
        printf("\n");
        return false;
    }
    else {
        printf("Error getting login ack.\n");
    }

    return false;
}

bool logout(){
    printf("goodbye\n");
    return false;
}

void getList(){
    printf("returned list\n");
    return;
};

void create_session(char* id){
    printf("created session\n");
    return;
};
bool join_session(char* id){
    printf("joined session %s\n",id);
    return true;
};
bool leave_session(){
    printf("left session\n");
    return true;
};
void send_message(char* message){
    printf("sent\n");
    return;
};


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

int getLenFromString(char* s) {
    return (int)((s[1] << 8) + s[0]);
}
