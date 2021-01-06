#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define ARRMAX 1000

struct loginuser {
    long type;
    int id_topic[ARRMAX];
    char name[ARRMAX];
    int subscription;
    char password[ARRMAX];
};

int main(int argc, char *argv[]) {
    
    int mid = msgget(0x160, 0644);
    struct loginuser _data;
    
    _data.type = 1;
    _data.id_topic[0] = 1;
    strcpy(_data.name, "Adrian");
    strcpy(_data.password, "dupa123");
    _data.subscription = 3;
    
    msgsnd(mid, &_data, sizeof(_data) - sizeof(_data.type), 0);

    return 0;
}