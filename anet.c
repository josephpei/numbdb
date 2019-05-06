#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "anet.h"


static void anetSetError(char *err, const char *fmt, ...)
{
    va_list ap;

    if (!err) return;
    va_start(ap, fmt);
    vsnprintf(err, ANET_ERR_LEN, fmt, ap);
    va_end(ap);
}

int anetNonBlock(char *err, int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    return ANET_OK;
}

int anetTcpNoDelay(char *err, int fd)
{
    int yes = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));

    return ANET_OK;
}

int anetRead(int fd, char *buf, int count)
{
    int nread, totlen = 0;
    while (totlen != count) {
        nread = read(fd, buf, count-totlen);
        if (nread == 0) return totlen;
        if (nread == -1) return -1;
        totlen += nread;
        buf += nread;
    }
    return totlen;
}

int anetWrite(int fd, char *buf, int count)
{
    int nwritten, totlen = 0;
    while (totlen != count) {
        nwritten = write(fd, buf, count-totlen);
        if (nwritten == 0) return totlen;
        if (nwritten == -1) return -1;
        totlen += nwritten;
        buf += nwritten;
    }
    return totlen;
}

static int _anetTcpServer(char *err, int port, char *bindaddr, int af, int backlog)
{
    int s = ANET_ERR, yes = 1;
    char portstr[6];
    struct addrinfo hints;
    struct addrinfo *servinfo, *p;

    snprintf(portstr, sizeof(portstr), "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = af;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(bindaddr, portstr, &hints, &servinfo);
    for (p = servinfo; p != NULL; p = p->ai_next) {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s == -1)
            continue;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        if (bind(s, p->ai_addr, p->ai_addrlen) == 0)
            break;
        close(s);
    }
    if (p == NULL) {
        anetSetError(err, "unable to bind socket, errno: %d", errno);
        return ANET_ERR;
    }
    listen(s, backlog);
    freeaddrinfo(servinfo);
    return s;
}

int anetTcpServer(char *err, int port, char *bindaddr)
{
    return _anetTcpServer(err, port, bindaddr, AF_INET, 511);
}

int anetTcp6Server(char *err, int port, char *bindaddr)
{
    return _anetTcpServer(err, port, bindaddr, AF_INET6, 511);
}

int anetTcpServerOld(char *err, int port, char *bindaddr)
{
    int s, on = 1;
    struct sockaddr_in sa;

    s = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bindaddr) {
        inet_aton(bindaddr, &sa.sin_addr);
    }

    bind(s, (struct sockaddr*)&sa, sizeof(sa));
    listen(s, 511);

    return s;
}

int anetAccept(char *err, int serversock, char *ip, int *port)
{
    int fd;
    struct sockaddr_in sa;
    unsigned int saLen;

    while (1) {
        saLen = sizeof(sa);
        fd = accept(serversock, (struct sockaddr *)&sa, &saLen);
        if (fd == -1) {
            if (errno == EINTR)
                continue;
        }
        break;
    }
    if (ip) strcpy(ip, inet_ntoa(sa.sin_addr));
    if (port) *port = ntohs(sa.sin_port);
    return fd;
}

#define ANET_CONNECT_NONE 0
#define ANET_CONNECT_NONBLOCK 1
static int anetTcpGenericConnect(char *err, char *addr, int port, int flags)
{
    int s = ANET_ERR, yes = 1;
    char portstr[6];
    struct addrinfo hints;
    struct addrinfo *servinfo, *p;

    snprintf(portstr, sizeof(portstr), "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(addr, portstr, &hints, &servinfo);
    for (p = servinfo; p != NULL; p = p->ai_next) {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        printf("cfd: %d\n", s);
        if (s == -1)
            continue;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        /* anetNonBlock(err, s); */
        /* TODO: NONBLOCK, errno EINPROGRESS, just return s */
        if (connect(s, p->ai_addr, p->ai_addrlen) != -1)
            break;
        close(s);
    }
    if (p == NULL)
        anetSetError(err, "creating socket: %s", strerror(errno));
    freeaddrinfo(servinfo);
    return s;
}

int anetTcpConnect(char *err, char *addr, int port)
{
    return anetTcpGenericConnect(err, addr, port, ANET_CONNECT_NONE);
}
