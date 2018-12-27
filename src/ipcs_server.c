#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>


#define MAXLINE 80

char *g_ServerSocketPath = "server.socket";
const int MAX_CLIENT_NUM = 20;

int CreateServerSocket(const char *serverName, size_t serverNameLen, int *serverFd)
{
    struct sockaddr_un serverAddr;
	int listenFd = 0;
    int result = 0;

    listenFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenFd < 0) {
        perror("socket error");
        exit(1);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_UNIX;
    strcpy(serverAddr.sun_path, serverName);

    /* 如果调用bind时文件已存在，则bind返回错误，所以先删除文件 */
    unlink(serverName);
    result = bind(listenFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (result < 0) {
        perror("bind error");
        exit(1);
    }
    printf("UNIX domain socket bound\n");

    result = listen(listenFd, MAX_CLIENT_NUM);
    if (result < 0) {
        perror("listen error");
        exit(1);
    }
    printf("Accepting connections ... \n");

    *serverFd = listenFd;

    return 0;
}

#define EPOLL_SIZE 20
#define EPOLL_RUN_TIMEOUT   500

int CreateServerEpoll(int serverFd, int *epollFd)
{
    struct epoll_event epollEvent;
    int tempFd = 0;

    tempFd = epoll_create(EPOLL_SIZE);

    epollEvent.events = EPOLLIN | EPOLLET;
    epollEvent.data.fd = serverFd;
    epoll_ctl(tempFd, EPOLL_CTL_ADD, serverFd, &epollEvent);

    *epollFd = tempFd;

    return 0;
}

int AcceptClientEpoll(int serverFd, int epollFd)
{
    struct sockaddr_un clientAddr;
	socklen_t clientAddrLen;
    int acceptFd = 0;
    struct epoll_event epollEvent;

    clientAddrLen = sizeof(clientAddr);
    acceptFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (acceptFd < 0) {
        perror("accept error");
    }

    epollEvent.events = EPOLLIN | EPOLLET;
    epollEvent.data.fd = acceptFd;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &epollEvent);

    return 0;
}

int HandleMessage(int clientFd)
{
    return 0;
}

int HandleServerEpollEvents(int serverFd, int epollFd)
{
    int events_num = 0;
    int i = 0;
    struct epoll_event events[EPOLL_SIZE];

    for (; ; ) {
        events_num = epoll_wait(epollFd, events, EPOLL_SIZE, EPOLL_RUN_TIMEOUT);
        for (i = 0; i < events_num; i++) {
            if (events[i].data.fd == serverFd) { 
                /* 有新的连接 */
                AcceptClientEpoll(serverFd, epollFd);
            } else if ((events[i].events & EPOLLIN) || (events[i].events & EPOLLPRI) || (events[i].events & EPOLLOUT)) {
                /* 有数据待接收或待发送 */
                HandleMessage(events[i].data.fd);
            } else {
                /* 错误处理 */
            }
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    struct sockaddr_un clientAddr;
	socklen_t clientAddrLen;
	int serverFd = 0;
    int clientFd = 0;
	char buf[MAXLINE] = {0};
    int msgLen = 0;
    int i;
    int epollFd = 0;

    CreateServerSocket(g_ServerSocketPath, strlen(g_ServerSocketPath), &serverFd);

    CreateServerEpoll(serverFd, &epollFd);

    HandleServerEpollEvents(serverFd, epollFd);

    close(serverFd);
    close(epollFd);

    return 0;
}

