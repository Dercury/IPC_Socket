/*
 * =====================================================================================
 *
 *       Filename:  ipcs_server.h
 *
 *    Description:  IPC socket server
 *
 *        Version:  1.0
 *        Created:  12/28/2018 10:33:49 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dercury (Jim), dercury@qq.com
 *   Organization:  Perfect World
 *
 * =====================================================================================
 */

#include "ipcs_server.h"

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


int IPCS_CreateServer(const char *serverName, ServerCallback serverHook)
{
    pthread_t threadId;
    IPCS_ServerThreadArg *threadArg = NULL;
    int result = 0;

    threadArg = (IPCS_ServerThreadArg *)malloc(sizeof(IPCS_ServerThreadArg));
    if (threadArg == NULL) {
        perror("malloc error");
        IPCS_WriteLog("Create Server: %s: malloc fail.", serverName);
        return IPCS_MALLOC_FAIL;
    }

    (void)memset(threadArg, 0, sizeof(IPCS_ServerThreadArg));
    snprintf(threadArg->name, sizeof(threadArg->name), "%s", serverName);
    threadArg->serverHook = serverHook;

    result = IPCS_CreateThread(IPCS_ServerRun, threadArg, &threadId);
    if (result != IPCS_OK) {
        free(threadArg);
        IPCS_WriteLog("Create Server: %s: create thread fail: %d.", serverName, result);
        return result;
    }
    IPCS_WriteLog("Create Server: %s: at thread %p success.", serverName, threadId);

    return IPCS_OK;
}

void *IPCS_ServerRun(void *arg)
{
    IPCS_ServerThreadArg *threadArg = (IPCS_ServerThreadArg *)arg;
	int serverFd = 0;
    int epollFd = 0;
    int result = 0;

    do {
        result = IPCS_CreateServerSocket(threadArg->name, &serverFd);
        if (result != IPCS_OK) {
            IPCS_WriteLog("Create server: %s socket fail: %d", threadArg->name, result);
            break;
        }
    
        result = IPCS_CreateServerEpoll(serverFd, &epollFd);
        if (result != IPCS_OK) {
            close(serverFd);
            IPCS_WriteLog("Create server: %s fd: %d epoll fail: %d", threadArg->name, serverFd, result);
            break;
        }
    
        result = IPCS_HandleServerEpollEvents(serverFd, epollFd, threadArg);
        if (result != IPCS_OK) {
            close(serverFd);
            close(epollFd);
            IPCS_WriteLog("Handle server: %s fd: %d epoll fd %d events fail: %d",
                    threadArg->name, serverFd, epollFd, result);
            break;
        }
    
        close(serverFd);
        close(epollFd);
    } while (0);

    free(threadArg);

    return NULL;
}

int IPCS_CreateServerSocket(const char *serverName, int *serverFd)
{
    struct sockaddr_un serverAddr;
	int listenFd = 0;
    int result = 0;

    listenFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenFd < 0) {
        perror("socket error");
        IPCS_WriteLog("Create server: %s socket fail: %d, errno: %d", serverName, listenFd, errno);
        return IPCS_SOCKET_FAIL;
    }

    (void)memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_UNIX;
    snprintf(serverAddr.sun_path, sizeof(serverAddr.sun_path), "%s", serverName);

    /* 如果调用bind时文件已存在，则bind返回错误，所以先删除文件 */
    unlink(serverName);
    result = bind(listenFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (result < 0) {
        close(listenFd);
        perror("bind error");
        IPCS_WriteLog("Bind server: %s socket: %d fail: %d, errno: %d", serverName, listenFd, result, errno);
        return IPCS_BIND_FAIL;
    }

    result = listen(listenFd, MAX_CLIENT_NUM);
    if (result < 0) {
        close(listenFd);
        perror("listen error");
        IPCS_WriteLog("Listen server: %s socket %d fail: %d, errno: %d", serverName, listenFd, result, errno);
        return IPCS_BIND_FAIL;
    }

    *serverFd = listenFd;
    IPCS_WriteLog("IPC socket server: %s socket: %d created.", serverName, *serverFd);

    return IPCS_OK;
}

int IPCS_CreateServerEpoll(int serverFd, int *epollFd)
{
    struct epoll_event epollEvent;
    int tempFd = 0;
    int result = 0;

    tempFd = epoll_create(EPOLL_SIZE);
    if (tempFd < 0) {
        perror("epoll create error");
        IPCS_WriteLog("Create server: %d epoll fail: %d, errno: %d", serverFd, tempFd, errno);
        return IPCS_EPOLL_CREATE_FAIL;
    }

    epollEvent.events = EPOLLIN | EPOLLET;
    epollEvent.data.fd = serverFd;
    result = epoll_ctl(tempFd, EPOLL_CTL_ADD, serverFd, &epollEvent);
    if (result < 0) {
        close(tempFd);
        perror("epoll ctl error");
        IPCS_WriteLog("Ctl server: %d epoll fail: %d, errno: %d", serverFd, result, errno);
        return IPCS_EPOLL_CTL_FAIL;
    }

    *epollFd = tempFd;
    IPCS_WriteLog("IPC socket server: %d epoll: %d created.", serverFd, *epollFd);

    return IPCS_OK;
}

int IPCS_HandleServerEpollEvents(int serverFd, int epollFd, IPCS_ServerThreadArg *threadArg)
{
    int events_num = 0;
    int i = 0;
    struct epoll_event events[EPOLL_SIZE];
    int result = IPCS_OK;

    for (; ; ) {
        events_num = epoll_wait(epollFd, events, EPOLL_SIZE, EPOLL_RUN_TIMEOUT);
        if (events_num < 0) {
            perror("epoll wait error");
            IPCS_WriteLog("Handle server: %d epoll: %d wait fail: %d, errno: %d",
                    serverFd, epollFd, events_num, errno);
            return IPCS_EPOLL_WAIT_FAIL;
        }

        for (i = 0; i < events_num; i++) {
            if (events[i].data.fd == serverFd) { 
                /* 有新的连接 */
                result = IPCS_ServerAcceptClient(serverFd, epollFd);
            } else if ((events[i].events & EPOLLIN) || 
                (events[i].events & EPOLLPRI) || 
                (events[i].events & EPOLLOUT)) {
                /* 有数据待接收或待发送 */
                result = IPCS_ServerHandleMessage(events[i].data.fd, threadArg);
            } else {
                /* 错误处理 */
                perror("epoll wait bad event");
                result = IPCS_EPOLL_BAD_EVENT;
            }

            if (result != IPCS_OK) {
                IPCS_WriteLog("Handle server: %d epoll: %d wait: %d bad events: %p, errno: %d",
                        serverFd, epollFd, events[i].data.fd, events[i].events, errno);
                return result;
            }
        }
    }

    return IPCS_OK;
}

int IPCS_ServerAcceptClient(int serverFd, int epollFd)
{
    struct sockaddr_un clientAddr;
	socklen_t clientAddrLen;
    int acceptFd = 0;
    struct epoll_event epollEvent;
    int result = 0;

    clientAddrLen = sizeof(clientAddr);
    acceptFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (acceptFd < 0) {
        perror("accept error");
        IPCS_WriteLog("Server: %d epoll: %d accept client fail: %d, errno: %d", serverFd, epollFd, acceptFd, errno);
        return IPCS_ACCEPT_FAIL;
    }

    epollEvent.events = EPOLLIN | EPOLLET;
    epollEvent.data.fd = acceptFd;
    result = epoll_ctl(epollFd, EPOLL_CTL_ADD, acceptFd, &epollEvent);
    if (result < 0) {
        perror("epoll ctl error");
        IPCS_WriteLog("Ctl server: %d epoll: %d add %d fail: %d, errno: %d", serverFd, epollFd, acceptFd, result, errno);
        return IPCS_EPOLL_CTL_FAIL;
    }

    return IPCS_OK;
}

int IPCS_ServerHandleMessage(int clientFd, IPCS_ServerThreadArg *threadArg)
{
    return IPCS_RecvMultiMsg(IPCS_SERVER, clientFd, threadArg);
}

/* 销毁服务端 */
int IPCS_DestroyServer(const char *serverName)
{
    return IPCS_OK;
}

/* 服务端响应消息，服务端响应请求的回调函数中使用 */
int IPCS_ServerSendMessage(int fd, IPCS_Message *msg)
{
    return IPCS_SendMessage(fd, msg);
}


