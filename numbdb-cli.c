#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "sds.h"
#include "zmalloc.h"
#include "anet.h"


static struct config {
    char *hostip;
    int hostport;
    int dbnum;
    int interactive;
} config;


static int cliConnect(int force)
{
    char err[ANET_ERR_LEN];
    static int fd = ANET_ERR;

    if (fd == ANET_ERR || force) {
        if (force) close(fd);
        fd = anetTcpConnect(err, config.hostip, config.hostport);
        anetTcpNoDelay(NULL, fd);
    }
    return fd;
}

#define LINE_BUFLEN 4096
static void repl()
{
    int argc, j, sfd, nw;
    char *line;
    sds *argv;

    while((line = readline("numbdb> ")) != NULL) {
        if (line[0] != '\0') {
            add_history(line);
            argv = sdssplitargs(line, &argc);

            if (argv == NULL) {
                printf("Invalid argument(s)\n");
                continue;
            } else if (argc > 0) {
                if (strcasecmp(argv[0], "quit") == 0 ||
                    strcasecmp(argv[0], "exit") == 0) {
                    exit(0);
                } else {
                    int err;
                }
            }

            sfd = cliConnect(0);

            for (j = 0; j < argc; j++) {
                printf("%d\n", sfd);
                nw = write(sfd, argv[j], strlen(argv[j]));
                printf("%d\n", nw);
                printf("%s\n", argv[j]);
                sdsfree(argv[j]);
            }
            zfree(argv);
        }

        free(line);
    }
}



int main(int argc, char *argv[])
{
    config.hostip = "127.0.0.1";
    config.hostport = 6378;
    config.dbnum = 0;
    config.interactive = 0;

    if (argc == 1) repl();
    return 0;
}
