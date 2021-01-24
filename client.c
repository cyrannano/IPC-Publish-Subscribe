#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h> 

#define ARRMAX 1000
#define TOPICSMAX 500
#define PASSMAX 10

int minusarray[ARRMAX] = {-1};

struct localUser {
    char name[PASSMAX];
    int id_topic[ARRMAX];
    int id;
} currentUser;

struct loginuser {
    long type;
    int id_topic[TOPICSMAX];
    char name[ARRMAX];
    int subscription[TOPICSMAX];
    char password[PASSMAX];
} userRemote;

struct imessage {
    long type;
    int topicId;
    struct loginuser user;
    char content[ARRMAX];
};

struct omessage {
    long type;
    int topicId;
    int senderId;
    char senderName[PASSMAX];
    char content[ARRMAX];
};

struct idData {
    long type;
    int idx;
};

struct subscriptionData {
    long type;
    int subscription[TOPICSMAX];
};

int connection;

void clearConsole() {
    for(int i = 0; i < 50; ++i) {
        printf("\n");
    }
}

void waitForUserInput() {
    printf("\nPress <enter> to continue...\n");
    getchar();
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
    userRemote.id_topic[0] = 1;
    strcpy(userRemote.name, uname);
    strcpy(userRemote.password, pass);
    userRemote.subscription[0] = 3;
    
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
    for(int i = 0; i < TOPICSMAX; ++i) {
        if(currentUser.id_topic[i] == -1) break;
        printf("%d\n", currentUser.id_topic[i]);
    }
    int topic;
    printf("Select one from given: ");
    scanf("%i", &topic);

    struct imessage _data;

    printf("Write a message: ");
    scanf("%s", _data.content);

    _data.type = 1;
    _data.topicId = topic;
    _data.user = userRemote;

    if(!fork())
        msgsnd(connection, &_data, sizeof(_data) - sizeof(_data.type), 0);

    clearConsole();
    printf("\nMessage sent successfully!\n");
    waitForUserInput();
}

void subscribeTopic() {
    clearConsole();
    printf("Type the topic ID, you would like to subscribe to\n");
    printf("Topic ID:");
    int tid;
    scanf("%i", &tid);

    struct subscriptionData _data;

    _data.type = 2;
    _data.subscription = tid;
    msgsnd(connection, &_data, sizeof(_data) - sizeof(_data.type), 0);

    printf("\nSuccessfully subscribed to topic %i!", tid);
    waitForUserInput();
}

void asyncMessageReceiver() {
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
            printf("Message: %s\n", _data.content);
            printf("--------------------------------\n\n");
            waitForUserInput();
        }
    }
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
    memcpy(currentUser.id_topic, minusarray, sizeof(minusarray));
    
    currentUser.id_topic[0] = 1;
    currentUser.id_topic[1] = 2;

    generateConnection();
    
    if(!fork()) {
        asyncMessageReceiver();
    }

    while(1) {
        clearConsole();
        printf("What would you like to do?\n");
        printf("[1] Send a message\n");
        printf("[2] Subscribe to a topic\n");
        printf("[3] Receive messages\n");
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
        }
    }

    return 0;
}