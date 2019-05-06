#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../sds.h"


int main(int argc, char *argv[])
{
    sds x, y;

    x = sdsnew("foo");
    printf("SDS x: %s, length: %ld\n", x, (long)sdslen(x));

    y = sdsnew("foo bar");
    if (sdscmp(x, y))
        printf("SDS x not eq y\n");
    else
        printf("SDS x eq y\n");

    return 0;
}
