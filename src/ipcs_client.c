/*
 * =====================================================================================
 *
 *       Filename:  ipcs_client.h
 *
 *    Description:  IPC socket client
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

#include "ipcs_client.h"

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
/* 创建同步客户端 */
int IPCS_CreateSyncClient(const char *clientName, const char *serverName, int *fd)
{
    int result = IPCS_OK;
    struct timeval timeout = {3, 0};    /* 3s */
    
    result = IPCS_CheckCreatingClient(clientName, serverName, fd);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Check creating sync client with bad params. Error: %d", result);
        return result;
    }
    
    result = IPCS_CreateClientSocket(clientName, serverName, fd);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Create sync client: %s, server: %s: create socket fail: %d.", clientName, serverName, result);
        return result;
    }

    result = setsockopt(*fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
    if (result != 0) {
        (void)close(*fd);
        perror("sync client setsockopt error");
        IPCS_WriteLog("Create sync client: %s, server: %s: set socket %d opt fail: %d, errno: %d.", clientName, serverName, *fd, result, errno);
        return result;
    }

    result = IPCS_AddSyncClientInfo(clientName, serverName, *fd);
    if (result != IPCS_OK) {
        (void)close(*fd);
        return result;
    }

    IPCS_WriteLog("Create sync client: %s, server: %s, socket: %d success.", clientName, serverName, *fd);

    return result;
}

int IPCS_CheckCreatingClient(const char *clientName, const char *serverName, int *fd)
{
    int result = IPCS_OK;

    if (fd == NULL) {
        IPCS_WriteLog("Check creating client with null fd.");
        return IPCS_PARAM_NULL;
    }

    result = IPCS_CheckItemName(clientName);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Check creating client with bad clientName. Error: %d", result);
        return result;
    }

    result = IPCS_CheckItemName(serverName);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Check creating client with bad serverName. Error: %d", result);
        return result;
    }

    return result;
}

int IPCS_CreateClientSocket(const char *clientName, const char *serverName, int *clientFd)
{
    struct sockaddr_un serverAddr;
    struct sockaddr_un clientAddr;
    int connectFd = 0;
    int result = 0;

    connectFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (connectFd < 0) {
        perror("client socket error");
        IPCS_WriteLog("Create client: %s server: %s socket fail: %d, errno: %d", clientName, serverName, connectFd, errno);
        return IPCS_SOCKET_FAIL;
    }

    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sun_family = AF_UNIX;
    snprintf(clientAddr.sun_path, sizeof(clientAddr.sun_path), "%s", clientName);
    unlink(clientAddr.sun_path);
    result = bind(connectFd, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
    if (result < 0) {
        (void)close(connectFd);
        perror("client bind error");
        IPCS_WriteLog("Bind client: %s server: %s socket: %d fail: %d, errno: %d", clientName, serverName, connectFd, result, errno);
        return IPCS_BIND_FAIL;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_UNIX;
    snprintf(serverAddr.sun_path, sizeof(serverAddr.sun_path), "%s", serverName);

    result = connect(connectFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (result < 0) {
        (void)close(connectFd);
        perror("client connect error");
        IPCS_WriteLog("Connect client: %s server: %s socket: %d fail: %d, errno: %d", clientName, serverName, connectFd, result, errno);
        return IPCS_CONNECT_FAIL;
    }

    *clientFd = connectFd;
    IPCS_WriteLog("IPC socket client: %s server: %s socket: %d created.", clientName, serverName, *clientFd);

    return IPCS_OK;
}

/******************************************************************************/
/* 同步调用 */
int IPCS_ClientSyncCall(int fd, IPCS_Message *sendMsg, IPCS_Message *recvMsg)
{
    int result = 0;

    result = IPCS_CheckClientSyncCall(fd, sendMsg, recvMsg);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Client: %d sync call with bad params: %d", fd, result);
        return result;
    }

    result = IPCS_SendMessage(fd, sendMsg);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Client: %d sync call: send msg fail: %d", fd, result);
        return result;
    }

    result = IPCS_RecvSingleMsg(fd, recvMsg);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Client: %d sync call: resv single msg fail: %d", fd, result);
        return result;
    }

    return result;
}

int IPCS_CheckClientSyncCall(int fd, IPCS_Message *sendMsg, IPCS_Message *recvMsg)
{
    int result = IPCS_OK;

    result = IPCS_IsItemExist(IPCS_SYNC_CLIENT, NULL, fd);
    if (result == 0) {
        IPCS_WriteLog("Client sync call with not exist fd: %d", fd);
        return IPCS_NOT_FOUND;
    }

    result = IPCS_CheckMessage(sendMsg);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Client: %d sync call with bad send msg: %d", fd, sendMsg);
        return result;
    }

    result = IPCS_CheckMessage(recvMsg);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Client: %d sync call with bad recv msg: %d", fd, recvMsg);
        return result;
    }

    return result;
}

/******************************************************************************/
/* 创建异步客户端 */
int IPCS_CreateAsynClient(const char *clientName, const char *serverName, ClientCallback clientHook, int *fd)
{
    pthread_t threadId;
    IPCS_AsynClientThreadArg *threadArg = NULL;
    int result = 0;
    
    /* NULL asyn client hook is allowed. */
    result = IPCS_CheckCreatingClient(clientName, serverName, fd);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Check creating asyn client with bad params. Error: %d", result);
        return result;
    }
    
    result = IPCS_CreateClientSocket(clientName, serverName, fd);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Create asyn client: %s, server: %s: create socket fail: %d.", clientName, serverName, result);
        return result;
    }

    threadArg = (IPCS_AsynClientThreadArg *)malloc(sizeof(IPCS_AsynClientThreadArg));
    if (threadArg == NULL) {
        (void)close(*fd);
        IPCS_WriteLog("Create asyn client: %s, server: %s, socket: %d: malloc fail.", clientName, serverName, *fd);
        return IPCS_MALLOC_FAIL;
    }
    (void)memset(threadArg, 0, sizeof(IPCS_AsynClientThreadArg));

    threadArg->fd = *fd;
    threadArg->clientHook = clientHook;
    result = IPCS_CreateThread(IPCS_AsynClientRun, threadArg, &threadId);
    if (result != IPCS_OK) {
        (void)close(*fd);
        free(threadArg);
        IPCS_WriteLog("Create asyn client: %s, server: %s, socket: %d: create thread fail: %d.", clientName, serverName, *fd, result);
        return result;
    }

    result = IPCS_AddAsynClientInfo(clientName, serverName, *fd, threadId, clientHook);
    if (result != IPCS_OK) {
        (void)close(*fd);
        free(threadArg);
        return result;
    }

    IPCS_WriteLog("Create asyn client: %s, server: %s, socket: %d: at thread %p success.", clientName, serverName, *fd, threadId);

    return result;
}

void *IPCS_AsynClientRun(void *arg)
{
    IPCS_AsynClientThreadArg *threadArg = (IPCS_AsynClientThreadArg *)arg;
    int result = 0;

    for (; ; ) {
        result = IPCS_RecvMultiMsg(IPCS_ASYN_CLIENT, threadArg->fd, threadArg);
        if (result != IPCS_OK) {
            break;
        }
    }

    free(threadArg);

    return NULL;
}

/******************************************************************************/
/* 异步调用 */
int IPCS_ClientAsynCall(int fd, IPCS_Message *sendMsg)
{
    int result = IPCS_OK;

    result = IPCS_CheckClientAsynCall(fd, sendMsg);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Client: %d asyn call with bad params: %d", fd, result);
        return result;
    }

    result = IPCS_SendMessage(fd, sendMsg);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Client: %d asyn call: send msg fail: %d", fd, result);
        return result;
    }

    return result;
}

int IPCS_CheckClientAsynCall(int fd, IPCS_Message *sendMsg)
{
    int result = IPCS_OK;

    result = IPCS_IsItemExist(IPCS_ASYN_CLIENT, NULL, fd);
    if (result == 0) {
        IPCS_WriteLog("Client asyn call with not exist fd: %d", fd);
        return IPCS_NOT_FOUND;
    }

    result = IPCS_CheckMessage(sendMsg);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Client: %d asyn call with bad send msg: %d", fd, sendMsg);
        return result;
    }

    return IPCS_OK;
}

/******************************************************************************/
/* 销毁客户端 */
int IPCS_DestroyClient(int fd)
{
    int result = 0;
    IPCS_ItemInfo itemInfo;

    (void)memset(&itemInfo, 0, sizeof(IPCS_ItemInfo));
    result = IPCS_FindItemsInfo(IPCS_SYNC_CLIENT, NULL, fd, &itemInfo);
    if (result != IPCS_OK) {
        result = IPCS_FindItemsInfo(IPCS_ASYN_CLIENT, NULL, fd, &itemInfo);
        if (result != IPCS_OK) {
            IPCS_WriteLog("Destroy client: %d not exist.", fd);
            return IPCS_OK;
        }
    }

    if (itemInfo.type == IPCS_ASYN_CLIENT) {
        result = pthread_cancel(itemInfo.pid);
        if (result != 0) {
            perror("pthread_cancel error");
            IPCS_WriteLog("Destroy asyn client: %d pthread_cancel: %p fail, errno: %d", fd, itemInfo.pid, errno);
            return result;
        }
    }

    result = close(fd);
    if (result != 0) {
        perror("close error");
        IPCS_WriteLog("Destroy client: %d close fail, errno: %d", fd, errno);
    } else {
        IPCS_WriteLog("Destroy client: %d success", fd);
    }

    return result;
}

/******************************************************************************/
int IPCS_AddSyncClientInfo(const char *clientName, const char *serverName, int fd)
{
    IPCS_ItemInfo info;
    int result = IPCS_OK;

    (void)memset(&info, 0, sizeof(IPCS_ItemInfo));

    info.type = IPCS_SYNC_CLIENT;
    (void)strcpy(info.name, clientName);
    (void)strcpy(info.peerName, serverName);
    info.fd = fd;

    result = IPCS_AddItemsInfo(&info);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Add sync client: %s, server: %s: socket: %d info fail: %d.", clientName, serverName, fd, result);
    }

    return result;
}

int IPCS_AddAsynClientInfo(const char *clientName, const char *serverName, int fd, pthread_t pid, ClientCallback hook)
{
    IPCS_ItemInfo info;
    int result = IPCS_OK;

    (void)memset(&info, 0, sizeof(IPCS_ItemInfo));

    info.type = IPCS_ASYN_CLIENT;
    (void)strcpy(info.name, clientName);
    (void)strcpy(info.peerName, serverName);
    info.fd = fd;
    info.pid = pid;
    info.hook = hook;

    result = IPCS_AddItemsInfo(&info);
    if (result != IPCS_OK) {
        IPCS_WriteLog("Add asyn client: %s, server: %s: socket: %d info fail: %d.", clientName, serverName, fd, result);
    }

    return result;
}

/******************************************************************************/

