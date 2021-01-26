#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
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
    
    printf("Hash: %d\n", (int)userRemote.type);
    printf("%d\n", sizeof(userRemote) - sizeof(userRemote.type));
    msgsnd(mid, &userRemote, sizeof(userRemote) - sizeof(userRemote.type), 0);

    struct idData uid_data;

    if(msgrcv(mid, &uid_data, sizeof(uid_data.idx), userRemote.type, 0) > 0) {
        printf("Assigned ID: %d\n", uid_data.idx);
        return uid_data.idx;
    }
    return -1;
}

int login() {
    char pass[PASSMAX];
    printf("username: ");
    scanf("%s", currentUser.name);    
    printf("password: ");
    scanf("%s", pass);

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
    scanf("%i", &tid);
    printf("\nSelect subscription duration (-1 = lifetime)\n");
    printf("Duration: ");
    scanf("%i", &sub);

    struct subscriptionData _data;

    _data.type = 3;
    _data.subscription = sub;
    _data.topicId = tid;
    msgsnd(connection, &_data, sizeof(_data) - sizeof(_data.type), 0);

    // printf("\nSuccessfully subscribed to topic %i!", tid);
    // waitForUserInput();
}

void* asyncMessageReceiver() {
    
    struct omessage _data;
    while(1) {
        if(msgrcv(connection, &_data, sizeof(_data) - sizeof(_data.type), 2, 0) > 0) {
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

void blockUser() {
    
}

int main(int argc, char *argv[]) {
    clearConsole();
    printf("Hello! Let's log you in!\n=================\n\n");
    int id;
    while((id = login()) < 0) {
        printf("Wrong username or password!\n");
        printf("Oops, let's try again:\n");
    }
    currentUser.id = id;
    memcpy(currentUser.id_topic, zeroarray, sizeof(zeroarray));

    generateConnection();
    
    pthread_t aMessageRcv;
    int err_0 = pthread_create(&aMessageRcv, NULL, asyncMessageReceiver, NULL);

    // if(!fork()) {
    //     asyncMessageReceiver();
    // }

    while(1) {
        clearConsole();
        printf("What would you like to do?\n");
        printf("[1] Send a message\n");
        printf("[2] Subscribe to a topic\n");
        printf("[3] Receive messages\n");
        printf("[4] Block user\n");
        printf("Select task: ");
        int d;
        scanf("%i", &d);
        switch(d) {
            case 1:
                sendMessage();
                break;
            case 2:
                subscribeTopic();
                break;
            case 3:
                asyncMessageReceiver();
                break;
            case 4:
                blockUser();
                break;
        }
    }

    return 0;
}





// 0 - REJESTRACJA UŻYTKOWNIKA
// 1 - REQUEST WYSŁANIA WIADOMOŚCI
// 2 - WIADOMOŚĆ PRZYCHODZĄCA
// 3 - DODANIE NOWEGO TEMATU DO SUBSKRYBCJI
