#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define ARRMAX 1000

// STRUCTS

struct client {
    int id;
    int id_topic[ARRMAX];
    int id_ignore[ARRMAX];
    char name[ARRMAX];
    int subscription;
    char password[ARRMAX];
};

// PATHS

char dataDir[] = "./serverData/";
char topicsFilePath[ARRMAX] = "";
char clientsFilePath[ARRMAX] = "";

// ARRAYS

int topics[ARRMAX];
struct client clients[ARRMAX];

// GLOBALS

int lastTopicId = 0;
int lastClientId = 0;

// FUNCTIONS

// GET DATA FROM .DATA FILES

void getPaths() {
    char topicsFileName[ARRMAX] = "topics.data";
    char clientsFileName[ARRMAX] = "clients.data";
    strcat(topicsFilePath, dataDir);
    strcat(topicsFilePath, topicsFileName);
    strcat(clientsFilePath, dataDir);
    strcat(clientsFilePath, clientsFileName);
}

void loadTopicsFromFile() {
    int fd = open(topicsFilePath, O_RDONLY|O_CREAT);
    char tmp[1];
    int tmpId = 0, i = 0, j = 1;
    while(read(fd, &tmp, 1) > 0) {
        if(tmp[0] == ';') {
            topics[i++] = tmpId;
            tmpId = 0;
            j = 1;
            continue;
        }
        tmpId = tmpId * j + atoi(tmp);
        j = 10;
    }
    lastTopicId = i;
    close(fd);
}

void loadClientsFromFile() {
    FILE *in;
    in = fopen(clientsFilePath, "wb+");
    
    if(in != NULL) {
        struct client c;
        while(fread(&c, sizeof(struct client), 1, in) > 0) {
            clients[lastClientId++] = c;
        }
    }

    fclose(in);
}

// SAVE DATA TO .DATA FILES

void addTopic(int topicId) {
    topics[lastTopicId++] = topicId;
    int fd = open(topicsFilePath, O_WRONLY|O_CREAT);
    char data[ARRMAX] = "";
    lseek(fd, 0, SEEK_END);
    sprintf(data, "%d;", topicId);
    write(fd, data, sizeof(topicId)+1);
    close(fd);
}

void addClient(struct client c) {
    clients[lastClientId++] = c;
    FILE *out;
    out = fopen(clientsFilePath, "a+");
    fwrite(&c, sizeof(struct client), 1, out);
    fclose(out);
    printf("User registered...\n");
}

void sendTopicToRecipients() {
    for(int i = 0; i < lastClientId; ++i) {

    }
}

struct loginuser {
    long type;
    int id_topic[ARRMAX];
    char name[ARRMAX];
    int subscription;
    char password[ARRMAX];
};

int authenticateUser(struct loginuser account, int id) {
    if(account.password == clients[id].password) {
        printf("User authenticated...\n");
        return 1;
    }
    printf("User authentication failed...\n");
    return 0;
}

void registerUser(struct loginuser account) {
    printf("Registering user...\n");
    struct client newclient;
    newclient.id = lastClientId;
    memcpy(newclient.id_topic, account.id_topic, sizeof(account.id_topic));
    strcpy(newclient.name, account.name);
    strcpy(newclient.password, account.password);
    newclient.subscription = account.subscription;
    addClient(newclient);
}

//  MAIN

int main(int argc, char *argv[]) {
    getPaths();
    loadTopicsFromFile();
    loadClientsFromFile();
    // for(int i = 0; i < lastTopicId; ++i) {
    //     printf("%i\n", topics[i]);
    // }
    // addTopic(2137);
    // for(int i = 0; i < lastTopicId; ++i) {
    //     printf("%i\n", topics[i]);
    // }

    // Receiving a message with id 1 = login/register request
    // Tutaj trzeba dorobić komunikację między procesem potomnym a macierzystym 
    // Myślałem o pamięci współdzielonej, ale jak nie pójdzie to kolejkami jakoś to zrobimy
    
    if(!fork()) {
        int mid = msgget(0x160, 0644|IPC_CREAT);
        struct loginuser _data;
        while(1) {
            if(msgrcv(mid, &_data, sizeof(_data) - sizeof(_data.type), 1, 0) > 0) {
                int logged = 0;
                printf("Data received...\n");
                for(int i = 0; i < lastClientId; ++i) {
                    if(clients[i].name == _data.name) {
                        printf("User found...\n");
                        if(authenticateUser(_data, clients[i].id)) {
                            logged = 1;
                            break;
                        }
                    }
                }
                if(!logged) {
                    printf("User not found...\n");
                    registerUser(_data);
                }
            }
            printf("%d\n", lastClientId);

        }
    }
    while(1) {
        if(lastClientId > 0) {
            printf("%s\n", clients[lastClientId].name);
        }
        printf("%d\n", lastClientId);
        sleep(1);
    }
    return 0;
}