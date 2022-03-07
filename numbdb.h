#pragma once

#include "ae.h"
#include "anet.h"
#include "zmalloc.h"
#include "dict.h"
#include "sds.h"


#define NUMBDB_OK 0
#define NUMBDB_ERR -1

#define NUMBDB_SERVERPORT 6378
#define NUMBDB_IOBUF_LEN 1024

#define LL_DEBUG 0
#define LL_VERBOSE 1
#define LL_NOTICE 2
#define LL_WARNING 3

#define LRU_BITS 24

#define ZSKIPLIST_MAXLEVEL 32
#define ZSKIPLIST_P 0.25

struct numbdbServer {
    int port;
    char *bindaddr;
    int fd;
    aeEventLoop *el;

    int verbosity;
    char *logfile;

};

typedef struct numbdbClient {
    int fd;
} numbdbClient;

typedef struct redisObject {
    unsigned type:4;
    unsigned encoding:4;
    unsigned lru:LRU_BITS;
    int refcount;
    void *ptr;
} robj;

typedef struct zskiplistNode {
    sds ele;
    double score;
    struct zskiplistNode *backward;
    struct zskiplistLevel {
        struct zskiplistNode *forward;
        unsigned int span;
    } level[];
} zskiplistNode;

typedef struct zskiplist {
    struct zskiplistNode *header, *tail;
    unsigned long length;
    int level;
} zskiplist;

typedef struct zset {
    dict *dict;
    zskiplist *zsl;
} zset;

void acceptHandler(aeEventLoop *el, int fd, void *privdata, int mask);
numbdbClient *createClient(int fd);
void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask);
void addReply(numbdbClient *c, robj *obj);
void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask);

zskiplist *zslCreate(void);
void zslFree(zskiplist *zsl);
zskiplistNode *zslInsert(zskiplist *zsl, double score, sds ele);
int zslDelete(zskiplist *zsl, double score, sds ele, zskiplistNode **node);


extern struct numbdbServer server;
