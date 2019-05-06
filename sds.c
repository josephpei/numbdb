#include <string.h>
#include "sds.h"
#include "zmalloc.h"


sds sdsnewlen(const void *init, size_t initlen)
{
    struct sdshdr *sh;

    sh = zmalloc(sizeof(struct sdshdr) + initlen + 1);
    sh->len = initlen;
    sh->free = 0;
    if (initlen) {
        if (init) memcpy(sh->buf, init, initlen);
        else memset(sh->buf, 0, initlen);
    }
    sh->buf[initlen] = '\0';

    return (char *)sh->buf;
}

sds sdsnew(const char *init)
{
    size_t initlen = (init == NULL) ? 0 : strlen(init);
    return sdsnewlen(init, initlen);
}

sds sdsempty()
{
    return sdsnewlen("", 0);
}

void sdsfree(sds s)
{
    if (s == NULL) return;
    zfree(s - sizeof(struct sdshdr));
}

size_t sdslen(const sds s)
{
    struct sdshdr *sh = (void *) (s - (sizeof(struct sdshdr)));
    return sh->len;
}

size_t sdsavail(sds s)
{
    struct sdshdr *sh = (void *) (s - (sizeof(struct sdshdr)));
    return sh->free;
}

int sdscmp(sds s1, sds s2)
{
    size_t l1, l2, minlen;
    int cmp;

    l1 = sdslen(s1);
    l2 = sdslen(s2);
    minlen = (l1 < l2) ? l1 : l2;
    cmp = memcmp(s1, s2, minlen);
    if (cmp == 0) return l1 - l2;
    return cmp;
}

static sds sdsMakeRoomFor(sds s, size_t addlen)
{
    struct sdshdr *sh, *newsh;
    size_t free = sdsavail(s);
    size_t len, newlen;

    if (free >= addlen) return s;
    len = sdslen(s);
    sh = (void *)(s - (sizeof(struct sdshdr)));
    newlen = (len + addlen) * 2;
    newsh = zrealloc(sh, sizeof(struct sdshdr) + newlen + 1);
    newsh->free = newlen - len;
    return newsh->buf;
}

sds sdscatlen(sds s, void *t, size_t len)
{
    struct sdshdr *sh;
    size_t curlen = sdslen(s);

    s = sdsMakeRoomFor(s, len);
    sh = (void *)(s - (sizeof(struct sdshdr)));
    memcpy(s + curlen, t, len);
    sh->len = curlen + len;
    sh->free = sh->free - len;
    s[curlen + len] = '\0';
    return s;
}

sds sdscat(sds s, char *t)
{
    return sdscatlen(s, t, strlen(t));
}

sds *sdssplitargs(char *line, int *argc)
{
    char *p = line;
    char *current = NULL;
    char **vector = NULL;

    *argc = 0;
    while(1) {
        while (*p && isspace(*p)) p++;
        if (*p) {
            int inq = 0;
            int done = 0;

            if (current == NULL) current = sdsempty();
            while (!done) {
                if (inq) {
                    
                } else {
                    switch (*p) {
                    case ' ':
                    case '\n':
                    case '\r':
                    case '\t':
                    case '\0':
                        done = 1;
                        break;
                    case '"':
                        inq = 1;
                        break;
                    default:
                        current = sdscatlen(current, p, 1);
                        break;
                    }
                }
                if (*p) p++; 
            }
            vector = zrealloc(vector, ((*argc) + 1) * sizeof(char *));
            vector[*argc] = current;
            (*argc)++;
            current = NULL;
        } else {
            return vector;
        }
    }
}
