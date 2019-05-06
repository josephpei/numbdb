#pragma once

#include <sys/types.h>

typedef char *sds;

struct sdshdr {
    long len;
    long free;
    char buf[];
};


sds sdsnew(const char *init);
sds sdsnewlen(const void *init, size_t initlen);
sds sdsempty();
void sdsfree(sds s);
size_t sdslen(const sds s);
size_t sdsavail(sds s);

int sdscmp(sds s1, sds s2);
sds sdscatlen(sds s, void *t, size_t len);
sds sdscat(sds s, char *t);

sds *sdssplitargs(char *line, int *argc);
