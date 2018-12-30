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


/* 创建同步客户端 */
int IPCS_CreateSyncClient(const char *clientName, const char *serverName, int *fd)
{
    int result = IPCS_OK;
    
    result = IPCS_CreateClientSocket(clientName, serverName, fd);

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
        perror("client bind error");
        IPCS_WriteLog("Bind client: %s server: %s socket: %d fail: %d, errno: %d", clientName, serverName, connectFd, result, errno);
        return IPCS_BIND_FAIL;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_UNIX;
    snprintf(clientAddr.sun_path, sizeof(clientAddr.sun_path), "%s", serverName);
    unlink(serverAddr.sun_path);

    result = connect(connectFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (result < 0) {
        perror("client connect error");
        IPCS_WriteLog("Connect client: %s server: %s socket: %d fail: %d, errno: %d", clientName, serverName, connectFd, result, errno);
        return IPCS_CONNECT_FAIL;
    }

    *clientFd = connectFd;
    IPCS_WriteLog("IPC socket client: %s server: %s socket: %d created.", clientName, serverName, *clientFd);

    return 0;
}

/* 同步调用 */
int IPCS_ClientSyncCall(int fd, IPCS_Message *sendMsg, IPCS_Message *recvMsg)
{
    int result = 0;
    void *streamBuf = NULL;
    unsigned int bufLen = IPCS_MESSAGE_MAX_LEN;
    ssize_t readLen = 0;

    result = IPCS_SendMessage(fd, sendMsg);
    if (result != IPCS_OK) {
        return result;
    }

    streamBuf = malloc(bufLen);
    if (streamBuf == NULL) {
        perror("malloc error");
        IPCS_WriteLog("Client: %d sync call: malloc fail.", fd);
        return IPCS_MALLOC_FAIL;
    }
    (void)memset(streamBuf, 0, bufLen);

    do {
        (void)memset(streamBuf, 0, bufLen);
        readLen = read(fd, streamBuf, bufLen);
        if (readLen <= 0) {
            perror("read error");
            IPCS_WriteLog("Client: %d sync call: read fail: %d, errno: %d", fd, readLen, errno);
            result = IPCS_READ_FAIL;
            break;
        }

        result = IPCS_StreamToMsg(streamBuf, readLen, recvMsg);
        if (result != IPCS_OK) {
            IPCS_WriteLog("Client: %d sync call: stream to msg fail: %d", fd, result);
            break;
        }
    } while (0);

    free(streamBuf);

    return result;
}

/* 创建异步客户端 */
int IPCS_CreateAsynClient(const char *clientName, const char *serverName, ClientCallback clientHook, int *fd)
{
    pthread_t threadId;
    IPCS_AsynClientThreadArg *threadArg = NULL;
    int result = 0;
    
    result = IPCS_CreateClientSocket(clientName, serverName, fd);
    if (result != IPCS_OK) {
        return result;
    }

    threadArg = (IPCS_AsynClientThreadArg *)malloc(sizeof(IPCS_AsynClientThreadArg));
    if (threadArg == NULL) {
        IPCS_WriteLog("Create asyn client: malloc fail.");
        return IPCS_MALLOC_FAIL;
    }
    (void)memset(threadArg, 0, sizeof(IPCS_AsynClientThreadArg));

    threadArg->fd = *fd;
    threadArg->clientHook = clientHook;
    result = IPCS_CreateThread(IPCS_AsynClientRun, threadArg, &threadId);

    return result;
}

void *IPCS_AsynClientRun(void *arg)
{
    IPCS_AsynClientThreadArg *threadArg = (IPCS_AsynClientThreadArg *)arg;
    int result = 0;

    for (; ; ) {
        result = IPCS_RecvMessage(IPCS_ASYN_CLIENT, threadArg->fd, threadArg);
        if (result != IPCS_OK) {
            break;
        }
    }

    free(threadArg);

    return NULL;
}

/* 异步调用 */
int IPCS_ClientAsynCall(int fd, IPCS_Message *sendMsg)
{
    return IPCS_SendMessage(fd, sendMsg);
}

/* 销毁客户端 */
int IPCS_DestroyClient(int fd)
{
    int result = 0;
    result = close(fd);
    if (result != 0) {
        perror("close error");
        IPCS_WriteLog("Destroy client: %d close fail, errno: %d", fd, errno);
    }

    return result;
}

