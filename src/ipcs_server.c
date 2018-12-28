#include "ipcs.h"

#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>


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

#define IPCS_MESSAGE_MAX_LEN    (32 * 1024)
#define IPCS_SERVER_NAME_MAX_LEN     256

typedef struct {
    char name[IPCS_SERVER_NAME_MAX_LEN];
    ServerResponseFunc responseProc;
} IPCS_ServerThreadArg;

int HandleMessage(int clientFd, IPCS_ServerThreadArg *threadArg)
{
    int result = 0;
    void *recvData = NULL;
    size_t recvDataLen = IPCS_MESSAGE_MAX_LEN;
    ssize_t msgLen = 0;

    recvData = malloc(recvDataLen);
    if (recvData == NULL) {
        printf("malloc error");
    }
    (void)memset(recvData, 0, recvDataLen);

    msgLen = read(clientFd, recvData, recvDataLen);

    result = threadArg->responseProc(clientFd, cmdId, recvData, recvDataLen);

    free(recvData);

    return 0;
}

int HandleServerEpollEvents(int serverFd, int epollFd, IPCS_ServerThreadArg *threadArg)
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
                HandleMessage(events[i].data.fd, threadArg);
            } else {
                /* 错误处理 */
            }
        }
    }

    return 0;
}

void *IPCS_ServerRun(void *arg)
{
    IPCS_ServerThreadArg *threadArg = (IPCS_ServerThreadArg *)arg;
	int serverFd = 0;
    int epollFd = 0;

    CreateServerSocket(threadArg->name, strlen(threadArg->name), &serverFd);

    CreateServerEpoll(serverFd, &epollFd);

    HandleServerEpollEvents(serverFd, epollFd, threadArg);

    close(serverFd);
    close(epollFd);
    free(arg);

    return;
}

int IPCS_CreateServer(const char *serverName, size_t serverNameLen, ServerResponseFunc serverResponseProc)
{
    pthread_t threadId;
    pthread_attr_t threadAttr;
    IPCS_ServerThreadArg *threadArg = NULL;

    threadArg = (IPCS_ServerThreadArg *)malloc(sizeof(IPCS_ServerThreadArg));
    if (threadArg == NULL) {
        return -1;
    }
    (void)memset(threadArg, 0, sizeof(IPCS_ServerThreadArg));

    pthread_attr_init(&threadAttr);
    /* 将threadAttr内相关属性设置为PTHREAD_CREATE_DETACHED，线程会变成unjoinable状态，
     * 则新线程不能用pthread_join来同步，且在退出时自行释放所占用的资源 */
    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);

    snprintf(threadArg->name, sizeof(threadArg->name), "%s", serverName);
    threadArg->responseProc = serverResponseProc;
    pthread_create(threadId, &threadAttr, IPCS_ServerRun, threadArg);

    pthread_attr_destroy(&threadAttr);

    return 0;
}

int main(int argc, char **argv)
{
    return 0;
}

/* 销毁服务端 */
int IPCS_DestroyServer(const char *serverName)
{
    return 0;
}

/* 服务端响应消息，服务端响应请求的回调函数中使用 */
unsigned int IPCS_RespondMessage(int fd, unsigned int cmdId, void *dataAddr, unsigned int dataLength)
{
    return 0;
}

