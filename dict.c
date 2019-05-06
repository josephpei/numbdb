#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "dict.h"
#include "zmalloc.h"


static int dict_can_resize = 1;
static unsigned int dict_force_resize_ratio = 5;

static unsigned long _dictNextPower(unsigned long size);
static int _dictKeyIndex(dict *d, const void *key);
static int _dictExpandIfNeeded(dict *d);
int dictRehash(dict *d, int n);


static void _dictReset(dictht *ht)
{
    ht->table = NULL;
    ht->size = 0;
    ht->sizemask = 0;
    ht->used = 0;
}

int _dictClear(dict *d, dictht *ht)
{
    unsigned long i;

    for (i = 0; i < ht->size && ht->used > 0; i++) {
        dictEntry *he, *nextHe;

        if ((he = ht->table[i]) == NULL) continue;
        while (he) {
            nextHe = he->next;
            dictFreeEntryKey(d, he);
            dictFreeEntryVal(d, he);
            zfree(he);
            ht->used --;
            he = nextHe;
        }
    }
    zfree(ht->table);
    _dictReset(ht);
    return DICT_OK;
}

dict *dictCreate(dictType *type, void *privDataPtr)
{
    dict *d = zmalloc(sizeof(*d));

    _dictReset(&d->ht[0]);
    _dictReset(&d->ht[1]);
    d->type = type;
    d->privdata = privDataPtr;
    d->rehashidx = -1;
    d->iterators = 0;
    return d;
}

void dictEmpty(dict *d)
{
    _dictClear(d, &d->ht[0]);
    _dictClear(d, &d->ht[1]);
    d->rehashidx = -1;
    d->iterators = 0;
}

void dictRelease(dict *d)
{
    _dictClear(d, &d->ht[0]);
    _dictClear(d, &d->ht[1]);
    zfree(d);
}

int dictResize(dict *d)
{
    int minimal;

    if (!dict_can_resize || dictIsRehashing(d)) return DICT_ERR;

    minimal = d->ht[0].used;
    if (minimal < DICT_HT_INITIAL_SIZE)
        minimal = DICT_HT_INITIAL_SIZE;
    return dictExpand(d, minimal);
}

int dictExpand(dict *d, unsigned long size)
{
    dictht n;
    unsigned long realsize = _dictNextPower(size);

    if (d->ht[0].used > size)
        return DICT_ERR;

    n.size = realsize;
    n.sizemask = realsize - 1;
    n.table = zmalloc(realsize * sizeof(dictEntry*));
    n.used = 0;

    memset(n.table, 0, realsize * sizeof(dictEntry*));

    if (d->ht[0].table == NULL) {
        d->ht[0] = n;
        return DICT_OK;
    }
    d->ht[1] = n;
    d->rehashidx = 0;
    return DICT_OK;
}

int dictAdd(dict *d, void *key, void *val)
{
    int index;
    dictEntry *entry;
    dictht *ht;

    if (dictIsRehashing(d))
        dictRehash(d, 1);
    if ((index = _dictKeyIndex(d, key)) == -1)
        return DICT_ERR;

    entry = zmalloc(sizeof(*entry));
    ht = dictIsRehashing(d) ? &d->ht[1] : &d->ht[0];
    entry->next = ht->table[index];
    ht->table[index] = entry;
    ht->used++;

    dictSetHashKey(d, entry, key);
    dictSetHashVal(d, entry, val);
    return DICT_OK;
}

dictEntry *dictFind(dict *d, const void *key)
{
    dictEntry *he;
    unsigned int h, idx, table;

    if (d->ht[0].size == 0) return NULL;
    if (dictIsRehashing(d))
        dictRehash(d, 1);
    h = dictHashKey(d, key);
    for (table = 0; table <= 1; table++) {
        idx = h & d->ht[table].sizemask;
        he = d->ht[table].table[idx];
        while (he) {
            if (key == he->key || dictCompareHashKeys(d, key, he->key))
                return he;
            he = he->next;
        }
        if (!dictIsRehashing(d))
            return NULL;
    }
    return NULL;
}

int dictReplace(dict *d, void *key, void *val)
{
    dictEntry *entry, auxentry;

    if (dictAdd(d, key, val) == DICT_OK)
        return 1;

    entry = dictFind(d, key);
    auxentry = *entry;
    dictSetHashVal(d, entry, val);
    dictFreeEntryVal(d, &auxentry);
    return 0;
}

void *dictFetchValue(dict *d, const void *key)
{
    dictEntry *he;

    he = dictFind(d, key);
    return he ? dictGetEntryVal(he) : NULL;
}

static int dictGenericDelete(dict *d, const void *key, int nofree)
{
    unsigned int h, idx;
    dictEntry *he, *prevHe;
    int table;

    if (d->ht[0].size == 0) return DICT_ERR;
    if (dictIsRehashing(d)) dictRehash(d, 1);
    h = dictHashKey(d, key);

    for (table = 0; table <= 1; table++) {
        idx = h & d->ht[table].sizemask;
        he = d->ht[table].table[idx];
        prevHe = NULL;
        while (he) {
            if (dictCompareHashKeys(d, key, he->key)) {
                if (prevHe)
                    prevHe->next = he->next;
                else
                    d->ht[table].table[idx] = he->next;
                if (!nofree) {
                    dictFreeEntryKey(d, he);
                    dictFreeEntryVal(d, he);
                }
                zfree(he);
                d->ht[table].used--;
                return DICT_OK;
            }
            prevHe = he;
            he = he->next;
        }
        if (!dictIsRehashing(d)) break;
    }
    return DICT_ERR;
}

int dictRehash(dict *d, int n)
{
    if (!dictIsRehashing(d))
        return 0;

    while (n--) {
        dictEntry *de, *nextde;

        if (d->ht[0].used == 0) {
            zfree(d->ht[0].table);
            d->ht[0] = d->ht[1];
            _dictReset(&d->ht[1]);
            d->rehashidx = -1;
            return 0;
        }

        while(d->ht[0].table[d->rehashidx] == NULL)
            d->rehashidx++;
        de = d->ht[0].table[d->rehashidx];
        while (de) {
            unsigned int h;

            nextde = de->next;
            h = dictHashKey(d, de->key) & d->ht[1].sizemask;
            de->next = d->ht[1].table[h];
            d->ht[1].table[h] = de;
            d->ht[0].used--;
            d->ht[1].used++;
            de = nextde;
        }
        d->ht[0].table[d->rehashidx] = NULL;
        d->rehashidx++;
    }
    return 1;
}

dictIterator *dictGetIterator(dict *d)
{
    dictIterator *iter = zmalloc(sizeof(*iter));

    iter->d = d;
    iter->table = 0;
    iter->index = -1;
    iter->entry = NULL;
    iter->nextEntry = NULL;
    return iter;
}

dictEntry *dictNext(dictIterator *iter)
{
    while (1) {
        if (iter->entry == NULL) {
            dictht *ht = &iter->d->ht[iter->table];
            if (iter->index == -1 && iter->table == 0)
                iter->d->iterators++;
            iter->index++;
            if (iter->index >= (signed) ht->size)
                break;
            iter->entry = ht->table[iter->index];
        } else {
            iter->entry = iter->nextEntry;
        }
        if (iter->entry) {
            iter->nextEntry = iter->entry->next;
            return iter->entry;
        }
    }
    return NULL;
}

void dictReleaseIterator(dictIterator *iter)
{
    if (!(iter->index == -1 && iter->table ==0))
        iter->d->iterators--;
    zfree(iter);
}

unsigned int dictIntHashFunction(unsigned int key)
{
    key += ~(key << 15);
    key ^=  (key >> 10);
    key +=  (key << 3);
    key ^=  (key >> 6);
    key += ~(key << 11);
    key ^=  (key >> 16);
    return key;
}

unsigned int dictGenHashFunction(const unsigned char *buf, int len)
{
    unsigned int hash = 5381;

    while (len--)
        hash = ((hash << 5) + hash) + (*buf++); /* hash * 33 + c */
    return hash;
}

static unsigned long _dictNextPower(unsigned long size)
{
    unsigned long i = DICT_HT_INITIAL_SIZE;

    if (size >= LONG_MAX) return LONG_MAX;
    while (1) {
        if (i >= size)
            return i;
        i *= 2;
    }
}

static int _dictKeyIndex(dict *d, const void *key)
{
    unsigned int h, idx, table;
    dictEntry *he;

    _dictExpandIfNeeded(d);
    h = dictHashKey(d, key);
    for (table = 0; table <= 1; table++) {
        idx = h & d->ht[table].sizemask;
        he = d->ht[table].table[idx];
        while (he) {
            if (dictCompareHashKeys(d, key, he->key))
                return -1;
            he = he->next;
        }
        if (1) break;
    }
    return idx;
}

static int _dictExpandIfNeeded(dict *d)
{
    if (d->ht[0].size == 0)
        return dictExpand(d, DICT_HT_INITIAL_SIZE);

    if (d->ht[0].used >= d->ht[0].size &&
        (dict_can_resize || d->ht[0].used / d->ht[0].size > dict_force_resize_ratio)) {
        return dictExpand(d, ((d->ht[0].size > d->ht[0].used) ?
                               d->ht[0].size : d->ht[0].used) * 2);
    }
    return DICT_OK;
}

#define DICT_STATS_VECTLEN 50
static void _dictPrintStatsHt(dictht *ht)
{
    unsigned long i, slots = 0, chainlen, maxchainlen = 0;
    unsigned long totchainlen = 0;
    unsigned long clvector[DICT_STATS_VECTLEN];

    if (ht->used == 0) {
        printf("No stats available for empty dictionaries\n");
        return;
    }

    for (i = 0; i < DICT_STATS_VECTLEN; i++)
        clvector[i] = 0;
    for (i = 0; i < ht->size; i++) {
        dictEntry *he;

        if (ht->table[i] == NULL) {
            clvector[0]++;
            continue;
        }
        slots++;
        chainlen = 0;
        he = ht->table[i];
        while (he) {
            chainlen++;
            he = he->next;
        }
        clvector[(chainlen < DICT_STATS_VECTLEN) ? chainlen : (DICT_STATS_VECTLEN-1)]++;
        if (chainlen > maxchainlen)
            maxchainlen = chainlen;
        totchainlen += chainlen;
    }
    printf("Hash table ststs:\n");
    printf(" table size: %ld\n", ht->size);
    printf(" number of elements: %ld\n", ht->used);
    printf(" different slots: %ld\n", slots);
    printf(" max chain length: %ld\n", maxchainlen);
    printf(" avg chain length (counted): %.02f\n", (float)totchainlen/slots);
    printf(" avg chain length (computed): %.02f\n", (float)ht->used/slots);
    printf(" Chain length distribution:\n");
    for (i = 0; i < DICT_STATS_VECTLEN-1; i++) {
        if (clvector[i] == 0) continue;
        printf("  %s%ld: %ld (%.02f%%)\n", (i == DICT_STATS_VECTLEN-1) ? ">= " : "",
               i, clvector[i], ((float)clvector[i]/ht->size)*100);
    }
}

void dictPrintStats(dict *d)
{
    _dictPrintStatsHt(&d->ht[0]);
}
