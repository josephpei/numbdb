/* file: test_fe.c */
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "../ae.h"

static const int MAX_SETSIZE = 64;
static const int MAX_BUFSIZE = 128;

static void
file_cb(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    char buf[MAX_BUFSIZE];
    int rc;

    rc = read(fd, buf, MAX_BUFSIZE);
    if (rc < 0)
    {
        aeStop(eventLoop);
        return;
    }
    else
    {
        buf[rc - 1] = '\0';  /* 最后一个字符是回车 */
    }
    printf("file_cb, read %s, fd %d, mask %d, clientData %s\n", buf, fd, mask, (char *)clientData);
}


int main(int argc, char *argv[])
{
    aeEventLoop *ae;
    long long id;
    int rc;

    ae = aeCreateEventLoop();
    if (!ae)
    {
        printf("create event loop error\n");
        goto err;
    }

    /* 添加文件IO事件 */
    rc = aeCreateFileEvent(ae, STDIN_FILENO, AE_READABLE, file_cb, (void *)"test ae file event");

    /* 添加定时器事件 */

    aeMain(ae);

    aeDeleteEventLoop(ae);
    return (0);

err:
    return (-1);
}
