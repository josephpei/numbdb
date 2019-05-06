#include <stdio.h>
#include <stdlib.h>

#include "../dict.h"
#include "../zmalloc.h"


int main(int argc, char *argv[])
{
    int i;
    int *k, *v;
    dict *d;
    dictEntry *entry;
    dictIterator *iter;
    dictType type = {
        dictIntHashFunction,
        NULL,
        zfree,
        zfree
    };


    d = dictCreate(&type, NULL);
    dictPrintStats(d);

    for (i = 0; i < 50; i++) {
        k = zmalloc(sizeof(int));
        v = zmalloc(sizeof(int));
        *k = i;
        *v = i * i;
        dictAdd(d, k, v);
    }

    while (dictRehash(d, 10)) { }
    dictPrintStats(d);

    iter = dictGetIterator(d);
    while((entry = dictNext(iter)) != NULL) {
        k = dictGetEntryKey(entry);
        v = dictGetEntryVal(entry);
        printf("Entry key: %d, val: %d\n", *k, *v);
    }

    dictReleaseIterator(iter);
    dictRelease(d);
    return 0;
}
