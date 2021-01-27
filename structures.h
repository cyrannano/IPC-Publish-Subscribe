#pragma once

#define ARRMAX 1000
#define TOPICSMAX 500
#define PASSMAX 10

struct idData {
    long type;
    int idx;
};


struct client {
    int id;
    int id_topic[ARRMAX];
    int id_ignore[ARRMAX];
    char name[ARRMAX];
    int subscription[ARRMAX];
    char password[ARRMAX];
};

struct loginuser {
    long type;
    int id_topic[TOPICSMAX];
    char name[PASSMAX];
    int subscription[TOPICSMAX];
    char password[PASSMAX];
};

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

struct subscriptionData {
    long type;
    int topicId;
    int subscription;
};

struct localUser {
    char name[PASSMAX];
    int id_topic[ARRMAX];
    int id;
};

struct blockPacket {
    long type;
    char name[PASSMAX];
};