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

/******************************************************************************/
int IPCS_CreateServer(const char *serverName, ServerCallback serverHook)
{
    pthread_t threadId;
    IPCS_ServerThreadArg *threadArg = NULL;
    int result = 0;

    result = IPCS_CheckCreatingServer(serverName, serverHook);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Check create server with bad params. Error: %d", result);
        return result;
    }

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

int IPCS_CheckCreatingServer(const char *serverName, ServerCallback serverHook)
{
    if (serverHook == NULL) {
        IPCS_WriteLog("Check creating server with null hook.");
        return IPCS_PARAM_NULL;
    }

    return IPCS_CheckItemName(serverName);
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
            (void)close(serverFd);
            IPCS_WriteLog("Create server: %s fd: %d epoll fail: %d", threadArg->name, serverFd, result);
            break;
        }

        result = IPCS_AddServerInfo(threadArg->name, serverFd, epollFd, pthread_self(), threadArg->serverHook);
        if (result != IPCS_OK) {
            (void)close(serverFd);
            (void)close(epollFd);
            break;
        }
    
        result = IPCS_HandleServerEpollEvents(serverFd, epollFd, threadArg);
        if (result != IPCS_OK) {
            (void)close(serverFd);
            (void)close(epollFd);
            IPCS_WriteLog("Handle server: %s fd: %d epoll fd %d events fail: %d",
                    threadArg->name, serverFd, epollFd, result);
            break;
        }
    
        (void)close(serverFd);
        (void)close(epollFd);
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
        (void)close(listenFd);
        perror("bind error");
        IPCS_WriteLog("Bind server: %s socket: %d fail: %d, errno: %d", serverName, listenFd, result, errno);
        return IPCS_BIND_FAIL;
    }

    result = listen(listenFd, MAX_CLIENT_NUM);
    if (result < 0) {
        (void)close(listenFd);
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
        (void)close(tempFd);
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
                IPCS_WriteLog("Server: %d epoll: %d handling events: %p from fd: %d ...", 
                               serverFd, epollFd, events[i].events, events[i].data.fd);
                /* 有数据待接收或待发送 */
                result = IPCS_ServerHandleMessage(events[i].data.fd, threadArg);
            } else {
                /* 错误处理 */
                perror("epoll wait bad event");
                result = IPCS_EPOLL_BAD_EVENT;
            }

            if (result != IPCS_OK) {
                IPCS_WriteLog("Server: %d epoll: %d got bad events: %p from fd: %d, errno: %d",
                        serverFd, epollFd, events[i].events, events[i].data.fd, errno);
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

    IPCS_WriteLog("Server: %d epoll: %d accept client %d success.", serverFd, epollFd, acceptFd);

    return IPCS_OK;
}

int IPCS_ServerHandleMessage(int clientFd, IPCS_ServerThreadArg *threadArg)
{
    return IPCS_RecvMultiMsg(IPCS_SERVER, clientFd, threadArg);
}

/******************************************************************************/
/* 销毁服务端 */
int IPCS_DestroyServer(const char *serverName)
{
    int result = 0;
    IPCS_ItemInfo itemInfo;
    
    result = IPCS_CheckItemName(serverName);
    if (result != IPCS_OK) {
        return result;
    }

    (void)memset(&itemInfo, 0, sizeof(IPCS_ItemInfo));
    result = IPCS_FindItemsInfo(IPCS_SERVER, serverName, 0, &itemInfo);
    if (result != IPCS_OK) {
        return IPCS_OK;
    }

    result = pthread_cancel(itemInfo.pid);
    if (result != 0) {
        perror("pthread_cancel error");
        IPCS_WriteLog("Destroy server: %s pthread_cancel: %p fail, errno: %d", serverName, itemInfo.pid, errno);
        return result;
    }

    result = close(itemInfo.epollFd);
    if (result != 0) {
        perror("close error");
        IPCS_WriteLog("Destroy server: %s epollFd: %d close fail, errno: %d", serverName, itemInfo.epollFd, errno);
        return result;
    } 
    
    result = close(itemInfo.fd);
    if (result != 0) {
        perror("close error");
        IPCS_WriteLog("Destroy server: %s sockfd: %d close fail, errno: %d", serverName, itemInfo.fd, errno);
    } else {
        IPCS_WriteLog("Destroy server: %s success", serverName);
    }

    return result;
}

/******************************************************************************/
/* 服务端响应消息，服务端响应请求的回调函数中使用 */
int IPCS_ServerSendMessage(int fd, IPCS_Message *msg)
{
    int result = IPCS_OK;

    result = IPCS_CheckSeverSendMsg(fd, msg);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Server send mgs to client: %d with bad params: %d", fd, result);
        return result;
    }

    result = IPCS_SendMessage(fd, msg);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Server send msg to client: %d fail: %d", fd, result);
        return result;
    }

    return result;
}

int IPCS_CheckSeverSendMsg(int fd, IPCS_Message *msg)
{
    int result = IPCS_OK;

    /* The input fd is not client fd, it's acceptFd. 
     * So it can't be checked by IPCS_IsItemExist. */

    result = IPCS_CheckMessage(msg);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Server send to client: %d with bad msg: %d", fd, result);
        return result;
    }

    return IPCS_OK;
}

/******************************************************************************/
int IPCS_AddServerInfo(const char *serverName, int fd, int epollFd, pthread_t pid, ServerCallback hook)
{
    IPCS_ItemInfo info;
    int result = IPCS_OK;

    (void)memset(&info, 0, sizeof(IPCS_ItemInfo));

    info.type = IPCS_SERVER;
    (void)strcpy(info.name, serverName);
    info.fd = fd;
    info.epollFd = epollFd;
    info.pid = pid;
    info.hook = hook;

    result = IPCS_AddItemsInfo(&info);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Add server: %s: socket: %d info fail: %d.", serverName, fd, result);
    }

    return result;
}

/******************************************************************************/

