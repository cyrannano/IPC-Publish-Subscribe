#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h> 
#include <pthread.h>

#include "structures.h"

struct localUser currentUser;
struct loginuser userRemote;

int connection;

int zeroarray[TOPICSMAX];

void clearConsole() {
    for(int i = 0; i < 50; ++i) {
        printf("\n");
    }
}

void waitForUserInput() {
    printf("\nPress <enter> to continue...\n");
    // char dummy;
    fflush(stdout);
    fgetc(stdin);
    // scanf(" %[^\n]", dummy);
    // Generuje błędy, potrzebna inna implementacja
}

void generateConnection() {
    char userIdChar[ARRMAX];
    char msgKeyChar[ARRMAX] = "9";
    sprintf(userIdChar, "%d", currentUser.id);
    strcat(msgKeyChar, userIdChar);
    key_t msgKey = atoi(msgKeyChar);
    
    connection = msgget(msgKey, 0644|IPC_CREAT);
}

int registerUser(char uname[ARRMAX], char pass[PASSMAX]) {
    int mid = msgget(0x160, 0644);
    
    srand(time(0)); 
    long ran = (long)(rand() % 25000);
    userRemote.type = ran;

    int tmp[ARRMAX] = {-1};
    memcpy(userRemote.id_topic, tmp, sizeof(tmp));
    strcpy(userRemote.name, uname);
    strcpy(userRemote.password, pass);
    
    // printf("Hash: %d\n", (int)userRemote.type);
    // printf("%d\n", sizeof(userRemote) - sizeof(userRemote.type));
    msgsnd(mid, &userRemote, sizeof(userRemote) - sizeof(userRemote.type), 0);

    struct idData uid_data;

    if(msgrcv(mid, &uid_data, sizeof(uid_data.idx), userRemote.type, 0) > 0) {
        // printf("Assigned ID: %d\n", uid_data.idx);
        return uid_data.idx;
    }
    return -1;
}

int login() {
    char pass[PASSMAX];
    printf("username: ");
    scanf("%s%*c", currentUser.name);    
    printf("password: ");
    scanf("%s%*c", pass);

    return registerUser(currentUser.name, pass);
}

// int register() {
//     char pass[PASSMAX];
//     char pass2[PASSMAX];
//     printf("username: ");
//     scanf("%s", currentUser.name);    
    
//     do {
//         printf("password: ");
//         scanf("%s", pass);
//         printf("confirm password: ");
//         scanf("%s", pass2);
//     }while(strcmp(pass, pass2) != 0)
    
//     printf("Select topics Id you want to subscribe to (type -1 when youre done): ");
//     int idt = 0;
//     for(int i = 0; i < ARRMAX; ++i) {
//         int id; 
//         if(id = -1) break;
//         scanf("%i", &id);
        
//     }
//     return registerUser(currentUser.name, pass);
// }

void sendMessage() {
    clearConsole();
    printf("Which topic ID would you like to use?\n");
    // for(int i = 0; i < TOPICSMAX; ++i) {
    //     if(currentUser.id_topic[i] == 0) break;
    //     printf("%d\n", currentUser.id_topic[i]);
    // }
    int topic;
    printf("Select one: ");
    scanf("%d", &topic);

    struct imessage _data;

    printf("Write a message: ");
    fflush(stdout);
    scanf(" %[^\n]%*c", _data.content);
    // printf("Write a message: ");
    // scanf("%s", _data.content);

    _data.type = 1;
    _data.topicId = topic;
    memcpy(&_data.user, &userRemote, sizeof(userRemote));

    msgsnd(connection, &_data, sizeof(_data) - sizeof(_data.type), 0);

    clearConsole();
    printf("\nMessage sent successfully!\n");
    waitForUserInput();
}

void subscribeTopic() {
    clearConsole();
    printf("Type the topic ID, you would like to subscribe to\n");
    printf("Topic ID: ");
    int tid, sub;
    scanf("%i%*c", &tid);
    printf("\nSelect subscription duration\n\t(-1 = lifetime)\n\t(0 = unsubscribe)\n");
    printf("Duration: ");
    scanf("%i%*c", &sub);

    struct subscriptionData _data;

    _data.type = 3;
    _data.subscription = sub;
    _data.topicId = tid;
    msgsnd(connection, &_data, sizeof(_data) - sizeof(_data.type), 0);

    printf("\nSuccessfully subscribed to topic %i!", tid);
    waitForUserInput();
}

int asyncBlock = 0;

void* asyncMessageReceiver() {
    
    struct omessage _data;
    while(1) {
        if(msgrcv(connection, &_data, sizeof(_data) - sizeof(_data.type), 2, 0) > 0) {
            while(asyncBlock){}
            clearConsole();
            printf("=================\n");
            printf("Message received\n");
            printf("=================\n");
            printf("Topic: %d\n", _data.topicId);
            printf("Sender: %s\n", _data.senderName);
            printf("Message:\n");
            printf("--------------------------------\n");
            printf("%s\n", _data.content);
            printf("--------------------------------\n\n");
            printf("Type 0 to continue: \n");
        }
    }
}
int interrupt;

void setInterrupt() {
    interrupt = 1;
}

void syncMessageReceiver() {
    clearConsole();
    printf("Which topic Id would you like to receive?\n");
    printf("Topic Id: ");
    int id;
    scanf("%i%*c", &id);
    printf("\n\nWaiting for incoming message from topic %i...\n", id);
    printf("\nPress Ctrl+Z to stop waiting\n\n");
    
    asyncBlock = 1;
    struct omessage _data;
    int received = 0;
    interrupt = 0;
    while(!received) {
        if(interrupt == 1) break;
        if(msgrcv(connection, &_data, sizeof(_data) - sizeof(_data.type), 2, 0) > 0) {
            if(_data.topicId != id) {
                msgsnd(connection, &_data, sizeof(_data) - sizeof(_data.type), 0);
                continue;
            }
            received = 1;
            clearConsole();
            printf("=================\n");
            printf("Message received\n");
            printf("=================\n");
            printf("Topic: %d\n", _data.topicId);
            printf("Sender: %s\n", _data.senderName);
            printf("Message:\n");
            printf("--------------------------------\n");
            printf("%s\n", _data.content);
            printf("--------------------------------\n\n");
        }
    }
    waitForUserInput();
}

void blockUser() {
    struct blockPacket _data;
    clearConsole();
    printf("=================\n");
    printf("Block user\n");
    printf("=================\n");
    printf("Type username you would like to block\n");
    printf("Username: ");
    scanf("%s%*c", _data.name);

    _data.type = 4;

    msgsnd(connection, &_data, sizeof(_data) - sizeof(_data.type), 0);
    clearConsole();
    printf("User %s blocked successfully\n", _data.name);
    waitForUserInput();
}

void unblockUser() {
    struct blockPacket _data;
    clearConsole();
    printf("=================\n");
    printf("Unblock user\n");
    printf("=================\n");
    printf("Type username you would like to unblock\n");
    printf("Username: ");
    scanf("%s%*c", _data.name);

    _data.type = 5;

    msgsnd(connection, &_data, sizeof(_data) - sizeof(_data.type), 0);
    clearConsole();
    printf("User %s unblocked successfully\n", _data.name);
    waitForUserInput();
}

pthread_t aMessageRcv;

void logout() {
    clearConsole();
    printf("Hello! Let's log you in!\n=================\n\n");
    int id;
    while((id = login()) < 0) {
        clearConsole();
        if(id == -9) {
            printf("\nLogin for current user has been blocked due to too many failed attempts!\n");
            printf("Wait about 1 minute before the next attempt\n");
            printf("Oops, let's try again:\n\n");
        }else {
            printf("\nWrong username or password!\n");
            printf("\nAllowed attempts left for this user: %i\n", id+14);
        }
    }
    currentUser.id = id;
    memcpy(currentUser.id_topic, zeroarray, sizeof(zeroarray));

    generateConnection();
    
    int err_0 = pthread_create(&aMessageRcv, NULL, asyncMessageReceiver, NULL);
    if(err_0) {
        printf("\nCouldn't open message receiver thread...\nShutting down...\n");
    }
}

int main(int argc, char *argv[]) {
    signal(SIGTSTP, setInterrupt);
    logout();
    // if(!fork()) {
    //     asyncMessageReceiver();
    // }

    while(1) {
        clearConsole();
        printf("Hello, %s\n", currentUser.name);
        printf("What would you like to do?\n");
        printf("[1] Send a message\n");
        printf("[2] Subscribe to a topic\n");
        printf("[3] Wait for a message\n");
        printf("[4] Block user\n");
        printf("[5] Unblock user\n");
        printf("[6] Log out\n");
        printf("[7] Exit\n");
        printf("Select task: ");
        asyncBlock = 0;
        int d;
        scanf("%i%*c", &d);
        switch(d) {
            case 1:
                sendMessage();
                break;
            case 2:
                subscribeTopic();
                break;
            case 3:
                syncMessageReceiver();
                break;
            case 4:
                blockUser();
                break;
            case 5:
                unblockUser();
                break;
            case 6:
                logout();
                break;
            case 7:
                return 0;
        }
    }

    return 0;
}





// 0 - REJESTRACJA UŻYTKOWNIKA
// 1 - REQUEST WYSŁANIA WIADOMOŚCI
// 2 - WIADOMOŚĆ PRZYCHODZĄCA
// 3 - DODANIE NOWEGO TEMATU DO SUBSKRYBCJI
