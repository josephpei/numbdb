#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>

#include "numbdb.h"


struct numbdbServer server;


static void numbdbLog(int level, const char *fmt, ...)
{
    va_list ap;
    FILE *fp;
    char *c = ".-*#";
    char buf[64];
    time_t now;

    if (level < server.verbosity) return;

    fp = (server.logfile == NULL) ? stdout : fopen(server.logfile, "a");

    va_start(ap, fmt);
    now = time(NULL);
    strftime(buf, 64, "%d %b %H:%M:%S", localtime(&now));
    fprintf(fp, "[%d] %s %c", (int)getpid(), buf, c[level]);
    vfprintf(fp, fmt, ap);
    fprintf(fp, "\n");
    fflush(fp);
    va_end(ap);

    if (server.logfile) fclose(fp);
}


static void initServerConfig() {
    server.port = NUMBDB_SERVERPORT;
    server.bindaddr = NULL;

    server.verbosity = LL_VERBOSE;
    server.logfile = NULL;
}

static void initServer()
{
    server.el = aeCreateEventLoop();
    server.fd = anetTcpServer(NULL, server.port, server.bindaddr);
    aeCreateFileEvent(server.el, server.fd, AE_READABLE, acceptHandler, NULL);
}

void acceptHandler(aeEventLoop *el, int fd, void *privdata, int mask)
{
    int cport, cfd;
    char cip[128];

    cfd = anetAccept(NULL, fd, cip, &cport);
    createClient(cfd);
}

numbdbClient *createClient(int fd)
{
    numbdbClient *c = zmalloc(sizeof(*c));

    anetNonBlock(NULL, fd);
    anetTcpNoDelay(NULL, fd);

    aeCreateFileEvent(server.el, fd, AE_READABLE, readQueryFromClient, c);

    return c;
}

void addReply(numbdbClient *c, robj *obj)
{
    aeCreateFileEvent(server.el, c->fd, AE_WRITABLE, sendReplyToClient, c);
}

void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask)
{
    char buf[NUMBDB_IOBUF_LEN];
    int nread;

    memset(buf, 0, NUMBDB_IOBUF_LEN);

    nread = read(fd, buf, NUMBDB_IOBUF_LEN);

    if (nread == -1) {
        if (errno == EAGAIN) {
            nread = 0;
        } else {
            /* freeClient(c); */
            return;
        }
    }
    if (nread) {
        numbdbLog(LL_NOTICE, "Readfrom client: %s\n", buf);
        write(fd, buf, nread);
    }
}

void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask)
{
    write(fd, privdata, strlen(privdata));
}

int main(int argc, char *argv[])
{
    initServerConfig();
    initServer();

    aeMain(server.el);
    aeDeleteEventLoop(server.el);

    return 0;
}
