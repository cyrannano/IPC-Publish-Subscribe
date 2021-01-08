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

struct loginuser {
    long type;
    int id_topic[TOPICSMAX];
    char name[ARRMAX];
    int subscription[TOPICSMAX];
    char password[PASSMAX];
};

struct omessage {

};

struct idData {
    long type;
    int idx;
};

int registerUser(char uname[ARRMAX], char pass[PASSMAX]) {
    int mid = msgget(0x160, 0644);
    struct loginuser _data;
    
    srand(time(0)); 
    long ran = (long)(rand() % 25000);
    _data.type = ran;

    int tmp[ARRMAX] = {-1};
    memcpy(_data.id_topic, tmp, sizeof(tmp));
    _data.id_topic[0] = 1;
    strcpy(_data.name, uname);
    strcpy(_data.password, pass);
    _data.subscription[0] = 3;
    
    printf("Hash: %d\n", (int)_data.type);
    printf("%d\n", sizeof(_data) - sizeof(_data.type));
    msgsnd(mid, &_data, sizeof(_data) - sizeof(_data.type), 0);

    struct idData uid_data;

    if(msgrcv(mid, &uid_data, sizeof(uid_data.idx), _data.type, 0) > 0) {
        printf("Assigned ID: %d\n", uid_data.idx);
        return uid_data.idx;
    }
    return -1;
}

int login() {
    char uname[ARRMAX];
    char pass[PASSMAX];
    printf("username: ");
    scanf("%s", uname);    
    printf("password: ");
    scanf("%s", pass);

    return registerUser(uname, pass);
}

int main(int argc, char *argv[]) {
    printf("Hello! Let's log you in!\n=================\n\n");
    while(login() < 0) {
        printf("Wrong username or password!\n");
        printf("Oops, let's try again:\n");
    }
    
    return 0;
}