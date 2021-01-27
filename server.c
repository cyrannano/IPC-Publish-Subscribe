#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "structures.h"

// STRUCTS

int lastSendRThreadId = 0;
pthread_t sendRequestThreads[ARRMAX];


// PATHS

char dataDir[] = "./serverData/";
char topicsFilePath[ARRMAX] = "";
char clientsFilePath[ARRMAX] = "";

// ARRAYS

int topics[TOPICSMAX];
struct client clients[ARRMAX];

// GLOBALS

int lastTopicId = 0;
int lastClientId = 0;

// FUNCTIONS

// GET DATA FROM .DATA FILES

int generateUserConnectionKey(struct client cur) {
    char userIdChar[ARRMAX];
    char msgKeyChar[ARRMAX] = "9";
    sprintf(userIdChar, "%d", cur.id);
    strcat(msgKeyChar, userIdChar);
    return (key_t)atoi(msgKeyChar);
}

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


// SAVE DATA TO .DATA FILES

void alterUserData(struct client toAlter) {
    printf("Altering user data...\n");
    FILE *out;
    out = fopen("./serverData/clients.data", "rw+");
    
    struct client f;
    while(fread(&f, sizeof(struct client), 1, out) != 0) {
        if(strcmp(f.name, toAlter.name) == 0) {
            printf("\tUser found...\n");
            fseek(out, -sizeof(struct client), SEEK_CUR);
            if(fwrite(&toAlter, sizeof(struct client), 1, out) > 0) {
                printf("\tUser altered...\n");
            }else {
                printf("\tUser alter failed...\n");
            }
            break;
        }
    }

    fclose(out);
}

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

int authenticateUser(struct loginuser account, int id) {
    if(!strcmp(account.password, clients[id].password)) {
        printf("User authenticated...\n");
        return 1;
    }
    printf("User authentication failed...\n");
    return 0;
}

int findUser(struct loginuser _data) {
    for(int i = 0; i < lastClientId; ++i) {
        printf("Searching: %s\n", clients[i].name);
        if(!strcmp(clients[i].name, _data.name)) {
            printf("User found...\n");
            if(authenticateUser(_data, clients[i].id)) {
                return i;
            }else {
                return -2;
            }
        }
    }
    return -1;
}

int findUserId(char _name[PASSMAX]) {
    for(int i = 0; i < lastClientId; ++i) {
        printf("Searching: %s\n", clients[i].name);
        if(!strcmp(clients[i].name, _name)) {
            printf("User found...\n");
            return clients[i].id;
        }
    }
    return -1;
}


void sendUserId(long conId, int uid) {
    int mid = msgget(0x160, 0644|IPC_CREAT);
    
    struct idData _data;
    _data.type = conId;
    _data.idx = uid;

    msgsnd(mid, &_data, sizeof(_data.idx), 0);
}


int checkIfBlocked(struct client c, int senderId) {
    for(int i = 0; i < ARRMAX; ++i) {
        if(c.id_ignore[i] == -1) break; 
        if(c.id_ignore[i] == senderId) return 1;
    }
    return 0;
}


int sendMessage(int topicId, int userId, char content[ARRMAX]) {
    int state = -1;
    for(int i = 0; i < lastClientId; ++i) {
        struct client cur = clients[i];
        printf("S\n");
        if(cur.id == userId) continue;
        printf("E\n");
        if(cur.subscription[topicId] == 0) continue;
        printf("N %i\n", cur.subscription[topicId]);
        // Sprawdzanie banów generuje błędy, trzeba jakoś poprawić
        if(checkIfBlocked(cur, userId)) continue;
        printf("D\n");

        if(cur.subscription[topicId] > 0) {
            clients[i].subscription[topicId]--;
        }
        
        key_t msgKey = generateUserConnectionKey(cur);
        
        int mid = msgget(msgKey, 0644|IPC_CREAT);

        struct omessage _data;

        printf("Sending to: %s\n", clients[i].name);

        strcpy(_data.content, content);
        _data.topicId = topicId;
        _data.senderId = userId;
        strcpy(_data.senderName, clients[i].name);
        _data.type = 2;
        alterUserData(cur);
        state = msgsnd(mid, &_data, sizeof(_data) - sizeof(_data.type), 0);
    }
    return state;
}

void* messageSendRequestHandler(void* mkey) {
    int mid = msgget((key_t)mkey, 0644|IPC_CREAT);
    struct imessage _data;
    struct subscriptionData _dataSub;
    struct blockPacket _dataBlock;
    
    char mkc[20];
    sprintf(mkc, "%d", (int)mkey);
    mkc[0] = ' ';
    int curUserId = atoi(mkc);

    while(1) {
        if(msgrcv(mid, &_data, sizeof(_data) - sizeof(_data.type), 1, IPC_NOWAIT) > 0) {
            printf("Message received...\n");
            printf("Message: %s\n", _data.content);
            int uid = findUser(_data.user);
            if(uid >= 0) {
                if(sendMessage(_data.topicId, uid, _data.content) > 0) {
                    printf("Message sent to reciptiens...\n");
                }else {
                    printf("Message failed to send...\n");
                }
            }else {
                printf("User not authorised!\n");
            }
        }
        if(msgrcv(mid, &_dataSub, sizeof(_dataSub) - sizeof(_dataSub.type), 3, IPC_NOWAIT) > 0) {
            printf("\n User subscription request received! \n");
            clients[curUserId].subscription[_dataSub.topicId] = _dataSub.subscription;
            alterUserData(clients[curUserId]);
        }
        if(msgrcv(mid, &_dataBlock, sizeof(_dataBlock) - sizeof(_dataBlock.type), 4, IPC_NOWAIT) > 0) {
            int _id = findUserId(_dataBlock.name);
            for(int i = 0; i < ARRMAX; ++i) {
                if(clients[curUserId].id_ignore[i] == -1 && i != ARRMAX - 1) {
                    clients[curUserId].id_ignore[i] = _id;
                    clients[curUserId].id_ignore[i + 1] = -1;
                    break;
                }
            }
            alterUserData(clients[curUserId]);
        }   
    }
}

void registerUser(struct loginuser account) {
    printf("Registering user...\n");
    struct client newclient;
    newclient.id = lastClientId;
    memcpy(newclient.id_topic, account.id_topic, sizeof(account.id_topic));
    memcpy(newclient.subscription, account.subscription, sizeof(account.subscription));
    strcpy(newclient.name, account.name);
    strcpy(newclient.password, account.password);
    newclient.id_ignore[0] = -1;
    addClient(newclient);
    pthread_create(&sendRequestThreads[lastSendRThreadId++], NULL, messageSendRequestHandler, (void *)generateUserConnectionKey(clients[lastClientId - 1]));
    sendUserId(account.type, newclient.id);
}

//  MAIN
void* userRegisterRequestHandler() {
    int mid = msgget(0x160, 0644|IPC_CREAT);
    struct loginuser _data;
    while(1) {
        if(msgrcv(mid, &_data, sizeof(_data) - sizeof(_data.type), 0, 0) > 0) {
            printf("Data received...%d\n", (int)_data.type);
            int uId = findUser(_data);
            if(uId == -1) {
                printf("User not found...\n");
                registerUser(_data);
            }else sendUserId(_data.type, uId);
        }
        printf("%d\n", lastClientId);
    }
}

void* printUsers() {
    while(1) {
        if(lastClientId > 0) {
            printf("%s\n", clients[lastClientId-1].name);
        }
        printf("%d\n", lastClientId);
        sleep(1);
    }
}


// + Zmieniłem tak, żeby tworzyć kolejkę dla każdego klienta
// + id kolejki to "9<id_użytkownika>"

// Zastanowić się czy na pewno robić kolejkę do każdego tematu, czy lepiej zrobić kolejkę dla każdego klienta
// Raczej dla każdego klienta, łatwiejsze w implementacji

void loadClientsFromFile() {
    FILE *in;
    in = fopen(clientsFilePath, "a+");
    
    if(in != NULL) {
        struct client c;
        while(fread(&c, sizeof(struct client), 1, in) > 0) {
            clients[lastClientId++] = c;
            pthread_create(&sendRequestThreads[lastSendRThreadId++], NULL, messageSendRequestHandler, (void *)generateUserConnectionKey(c));
        }
    }

    fclose(in);
}


int main(int argc, char *argv[]) {
    getPaths();
    loadTopicsFromFile();
    loadClientsFromFile();

    pthread_t lrrequest;
    int err_0 = pthread_create(&lrrequest, NULL, userRegisterRequestHandler, NULL);

    // pthread_t pusers;
    // int err_1 = pthread_create(&pusers, NULL, printUsers, NULL);
    
    
    // replaced forks with threads, register/login works, authentication works, reading/writing to files works, TODO: creating data folder

    while(1) {
        sleep(1);
    }

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
 


    return 0;
}