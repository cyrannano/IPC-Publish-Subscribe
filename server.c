#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "structures.h"

// STRUCTS

int lastSendRThreadId = 0;
pthread_t sendRequestThreads[ARRMAX];


// PATHS

char dataDir[] = "./serverData/";
char topicsFilePath[ARRMAX] = "";
char clientsFilePath[ARRMAX] = "";

// ARRAYS
int arrMaxEmptyArray[ARRMAX];
struct client clients[ARRMAX];
int loginBlock[ARRMAX];
int failedLoginAttempts[ARRMAX];

// GLOBALS
int _log = 0;
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


// SAVE DATA TO .DATA FILES

void alterUserData(struct client toAlter) {
    if(_log) printf("Altering user data...\n");
    FILE *out;
    out = fopen("./serverData/clients.data", "rw+");
    
    struct client f;
    while(fread(&f, sizeof(struct client), 1, out) != 0) {
        if(strcmp(f.name, toAlter.name) == 0) {
            if(_log) printf("\tUser found...\n");
            fseek(out, -sizeof(struct client), SEEK_CUR);
            if(fwrite(&toAlter, sizeof(struct client), 1, out) > 0) {
                if(_log) printf("\tUser altered...\n");
            }else {
                if(_log) printf("\tUser alter failed...\n");
            }
            break;
        }
    }

    fclose(out);
}

void addClient(struct client c) {
    clients[lastClientId++] = c;
    FILE *out;
    out = fopen(clientsFilePath, "a+");
    fwrite(&c, sizeof(struct client), 1, out);
    fclose(out);
    if(_log) printf("User registered...\n");
}

int authenticateUser(struct loginuser account, int id) {
    if(!strcmp(account.password, clients[id].password)) {
        if(_log) printf("User authenticated...\n");
        return 1;
    }
    if(_log) printf("User authentication failed...\n");
    return 0;
}

int findUser(struct loginuser _data) {
    for(int i = 0; i < lastClientId; ++i) {
        if(_log) printf("Searching: %s\n", clients[i].name);
        if(!strcmp(clients[i].name, _data.name)) {
            if(_log) printf("User found...\n");
            if(loginBlock[i] > 0) return -9;
            if(authenticateUser(_data, clients[i].id)) {
                failedLoginAttempts[i] = 0;
                return i;
            }else {
                if(failedLoginAttempts[i] > 2) {
                    if(_log) printf("Login for user %s has been blocked\n", clients[i].name);
                    loginBlock[i] = 60;
                    failedLoginAttempts[i] = 0;
                    return -9;
                }else {
                    if(_log) printf("User: %s failed login attempts: %i\n", clients[i].name, failedLoginAttempts[i]+1);
                    failedLoginAttempts[i]++;
                    return failedLoginAttempts[i]*(-1) - 10;
                }
            }
        }
    }
    return -1;
}

int findUserId(char _name[PASSMAX]) {
    for(int i = 0; i < lastClientId; ++i) {
        if(_log) printf("Searching: %s\n", clients[i].name);
        if(!strcmp(clients[i].name, _name)) {
            if(_log) printf("User found...\n");
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
        if(_log) printf("S\n");
        if(cur.id == userId) continue;
        if(_log) printf("E\n");
        if(cur.subscription[topicId] == 0) continue;
        if(_log) printf("N\n");
        // Sprawdzanie banów generuje błędy, trzeba jakoś poprawić
        if(checkIfBlocked(cur, userId)) continue;
        if(_log) printf("D\n");

        if(cur.subscription[topicId] > 0) {
            clients[i].subscription[topicId]--;
        }
        
        key_t msgKey = generateUserConnectionKey(cur);
        
        int mid = msgget(msgKey, 0644|IPC_CREAT);

        struct omessage _data;

        if(_log) printf("Sending to: %s\n", clients[i].name);

        strcpy(_data.content, content);
        _data.topicId = topicId;
        _data.senderId = userId;
        strcpy(_data.senderName, clients[userId].name);
        _data.type = 2;
        alterUserData(cur);
        state = msgsnd(mid, &_data, sizeof(_data) - sizeof(_data.type), 0);
    }
    return state;
}

void* messageSendRequestHandler(void* mkey) {
    int mid = msgget((uintptr_t)mkey, 0644|IPC_CREAT);
    struct imessage _data;
    struct subscriptionData _dataSub;
    struct blockPacket _dataBlock;
    
    char mkc[20];
    sprintf(mkc, "%d", (int)(uintptr_t)mkey);
    mkc[0] = ' ';
    int curUserId = atoi(mkc);

    while(1) {
        if(msgrcv(mid, &_data, sizeof(_data) - sizeof(_data.type), 1, IPC_NOWAIT) > 0) {
            if(_log) printf("Message received...\n");
            if(_log) printf("Message: %s\n", _data.content);
            int uid = findUser(_data.user);
            if(uid >= 0) {
                if(sendMessage(_data.topicId, uid, _data.content) > 0) {
                    if(_log) printf("Message sent to reciptiens...\n");
                }else {
                    if(_log) printf("Message failed to send...\n");
                }
            }else {
                if(_log) printf("User not authorised!\n");
            }
        }
        if(msgrcv(mid, &_dataSub, sizeof(_dataSub) - sizeof(_dataSub.type), 3, IPC_NOWAIT) > 0) {
            if(_log) printf("\n User subscription request received! \n");
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
                if(clients[curUserId].id_ignore[i] == _id) {
                    break;
                }
            }
            alterUserData(clients[curUserId]);
        }
        if(msgrcv(mid, &_dataBlock, sizeof(_dataBlock) - sizeof(_dataBlock.type), 5, IPC_NOWAIT) > 0) {
            int _id = findUserId(_dataBlock.name);
            int found = 0;
            for(int i = 0; i < ARRMAX; ++i) {
                if(clients[curUserId].id_ignore[i] == _id && !found) {
                    found = 1;
                    continue;
                }

                if(!found && clients[curUserId].id_ignore[i] == -1) break;

                if(found) {
                    clients[curUserId].id_ignore[i - 1] = clients[curUserId].id_ignore[i];
                    if(clients[curUserId].id_ignore[i] == -1) {
                        clients[curUserId].id_ignore[i] = 0;
                        break;
                    }
                }
            }       
            alterUserData(clients[curUserId]);
        }  
    }
}

void registerUser(struct loginuser account) {
    if(_log) printf("Registering user...\n");
    struct client newclient;
    newclient.id = lastClientId;
    memcpy(newclient.id_topic, account.id_topic, sizeof(account.id_topic));
    memcpy(newclient.subscription, account.subscription, sizeof(account.subscription));
    strcpy(newclient.name, account.name);
    strcpy(newclient.password, account.password);
    newclient.id_ignore[0] = -1;
    addClient(newclient);
    pthread_create(&sendRequestThreads[lastSendRThreadId++], NULL, messageSendRequestHandler, (void *)(uintptr_t)generateUserConnectionKey(clients[lastClientId - 1]));
    sendUserId(account.type, newclient.id);
}

//  MAIN
void* userRegisterRequestHandler() {
    int mid = msgget(0x160, 0644|IPC_CREAT);
    struct loginuser _data;
    while(1) {
        if(msgrcv(mid, &_data, sizeof(_data) - sizeof(_data.type), 0, 0) > 0) {
            if(_log) printf("Data received...%d\n", (int)_data.type);
            int uId = findUser(_data);
            if(uId == -1) {
                if(_log) printf("User not found...\n");
                registerUser(_data);
            }else sendUserId(_data.type, uId);
        }
        if(_log) printf("%d\n", lastClientId);
    }
}

void loadClientsFromFile() {
    FILE *in;
    in = fopen(clientsFilePath, "a+");
    
    if(in != NULL) {
        struct client c;
        while(fread(&c, sizeof(struct client), 1, in) > 0) {
            clients[lastClientId++] = c;
            pthread_create(&sendRequestThreads[lastSendRThreadId++], NULL, messageSendRequestHandler, (void *)(uintptr_t)generateUserConnectionKey(c));
        }
    }

    fclose(in);
}

void makeFolderData() {
    struct stat st = {0};
    if(stat(dataDir, &st) == -1) {
        mkdir(dataDir, 0700);
    }
}

void* refreshUserLoginBlock() {
    while(1) {
        sleep(1);
        for(int i = 0; i < ARRMAX; ++i) {
            if(loginBlock[i] == 1)
                if(_log) printf("Login block for user %s has expired\n", clients[i].name);
            if(loginBlock[i] > 0) loginBlock[i]--;
            
        }
    }
}

int main(int argc, char *argv[]) {
    getPaths();
    makeFolderData();
    loadClientsFromFile();

    if(argc > 1) {
        if(strcmp(argv[1], "-l ")) {
            _log = 1;
            printf("Log enabled!\n");
        }
    }
    pthread_t lrrequest;
    int err_0 = pthread_create(&lrrequest, NULL, userRegisterRequestHandler, NULL);
    if(err_0) {
        if(_log) printf("\nCouldn't open user loin/register thread...\nShutting down...\n");
        return 0;
    }

    pthread_t rlerror;
    int err_1 = pthread_create(&rlerror, NULL, refreshUserLoginBlock, NULL);
    if(err_1) {
        if(_log) printf("\nCouldn't open user refresh blocked users array thread...\nShutting down...\n");
        return 0;
    }
    
    while(1) {
        sleep(1);
    }


    return 0;
}