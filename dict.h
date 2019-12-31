#pragma once

#define DICT_OK 0
#define DICT_ERR -1
#define DICT_HT_INITIAL_SIZE 4

typedef struct dictEntry {
    void *key;
    void *val;
    struct dictEntry *next;
} dictEntry;

typedef struct dictType {
    unsigned int (*hashFunction)(const void *key);
    int (*keyCompare)(const void *key1, const void *key2);
    void (*keyDestructor)(void *key);
    void (*valDestructor)(void *obj);
} dictType;

typedef struct dictht {
    dictEntry **table;
    unsigned long size;
    unsigned long sizemask;
    unsigned long used;
} dictht;

typedef struct dict {
    dictType *type;
    void *privdata;
    dictht ht[2];
    int rehashidx;
    int iterators;
} dict;

typedef struct dictIterator {
    dict *d;
    int table;
    int index;
    dictEntry *entry, *nextEntry;
} dictIterator;


#define dictGetEntryKey(he) ((he)->key)
#define dictGetEntryVal(he) ((he)->val)
#define dictIsRehashing(d) ((d)->rehashidx != -1)

#define dictSetHashKey(d, entry, _key_) do { \
    entry->key = (_key_); \
} while(0)

#define dictSetHashVal(d, entry, _val_) do { \
    entry->val = (_val_); \
} while(0)

#define dictFreeEntryKey(d, entry) \
    if ((d)->type->keyDestructor) \
        (d)->type->keyDestructor((entry)->key)

#define dictFreeEntryVal(d, entry) \
    if ((d)->type->valDestructor) \
        (d)->type->valDestructor((entry)->val)

#define dictHashKey(d, key) (d)->type->hashFunction(key)

#define dictCompareHashKeys(d, key1, key2) \
    (((d)->type->keyCompare) ? \
    (d)->type->keyCompare(key1, key2) : \
     (key1) == (key2))

dict *dictCreate(dictType *type, void *privDataptr);
int dictResize(dict *d);
int dictExpand(dict *d, unsigned long size);
void dictEmpty(dict *d);
void dictRelease(dict *d);
int dictAdd(dict *d, void *key, void *val);
int dictDelete(dict *d, void *key);
dictEntry *dictFind(dict *d, const void *key);
void *dictFetchValue(dict *d, const void *key);
dictIterator *dictGetIterator(dict *d);
dictEntry *dictNext(dictIterator *iter);
void dictReleaseIterator(dictIterator *iter);
int dictRehash(dict *d, int n);

void dictPrintStats(dict *d);


unsigned int dictIntHashFunction(unsigned int key);
unsigned int dictGenHashFunction(const unsigned char *buf, int len);
