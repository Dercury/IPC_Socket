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
    int result = 0;
    
    result = IPCS_CreateClientSocket(clientName, serverName, fd);

    return 0;
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
        exit(1);
    }

    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sun_family = AF_UNIX;
    snprintf(clientAddr.sun_path, sizeof(clientAddr.sun_path), "%s", clientName);
    unlink(clientAddr.sun_path);
    result = bind(connectFd, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
    if (result < 0) {
        perror("client bind error");
        exit(1);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_UNIX;
    snprintf(clientAddr.sun_path, sizeof(clientAddr.sun_path), "%s", serverName);
    unlink(serverAddr.sun_path);

    result = connect(connectFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (result < 0) {
        perror("client connect error");
        exit(1);
    }

    *clientFd = connectFd;

    return 0;
}

/* 同步调用 */
int IPCS_SyncCall(int fd, IPCS_Message *sendMsg, IPCS_Message *recvMsg)
{
    int result = 0;
    void *streamBuf = NULL;
    unsigned int bufLen = IPCS_MESSAGE_MAX_LEN;
    ssize_t writeLen = 0;
    ssize_t readLen = 0;

    streamBuf = malloc(bufLen);
    if (streamBuf == NULL) {
        IPCS_WriteLog("Sync call: malloc fail.");
        return IPCS_MALLOC_FAIL;
    }
    (void)memset(streamBuf, 0, bufLen);

    do {
        result = IPCS_MsgToStream(sendMsg, streamBuf, &bufLen);
        if (result != IPCS_OK) {
            IPCS_WriteLog("Sync call: msg to stream fail: %d", result);
            break;
        }

        writeLen = write(fd, streamBuf, bufLen);
        if (writeLen <= 0) {
            IPCS_WriteLog("Sync call: write fd: %d fail: %d, errno: %d", fd, writeLen, errno);
            result = IPCS_WRITE_FAIL;
            break;
        }

        bufLen = IPCS_MESSAGE_MAX_LEN;
        (void)memset(streamBuf, 0, bufLen);
        readLen = read(fd, streamBuf, bufLen);
        if (readLen <= 0) {
            IPCS_WriteLog("Sync call: read fd: %d fail: %d, errno: %d", fd, readLen, errno);
            result = IPCS_READ_FAIL;
            break;
        }

        result = IPCS_StreamToMsg(streamBuf, readLen, recvMsg);
        if (result != IPCS_OK) {
            IPCS_WriteLog("Sync call: stream to msg fail: %d", result);
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

    threadArg = (IPCS_AsynClientThreadArg *)malloc(sizeof(IPCS_AsynClientThreadArg));
    if (threadArg == NULL) {
        return -1;
    }
    (void)memset(threadArg, 0, sizeof(IPCS_AsynClientThreadArg));

    threadArg->fd = *fd;
    threadArg->clientHook = clientHook;
    IPCS_CreateThread(IPCS_AsynClientRun, threadArg, &threadId);

    return 0;
}

void *IPCS_AsynClientRun(void *arg)
{
    IPCS_AsynClientThreadArg *threadArg = (IPCS_AsynClientThreadArg *)arg;
	int serverFd = 0;
    unsigned int cmdId = 0;
    void *recvData = NULL;
    unsigned int recvDataLen = IPCS_MESSAGE_MAX_LEN;
    ssize_t msgLen = 0;
    int result = 0;
    IPCS_Message msg;

    recvData = malloc(recvDataLen);
    if (recvData == NULL) {
        IPCS_WriteLog("Asyn client run: malloc fail.");
        return NULL;
    }

    for (; ; ) {
        (void)memset(recvData, 0, recvDataLen);
        msgLen = read(threadArg->fd, recvData, recvDataLen);
        if (msgLen < 0) {
            if (errno != EAGAIN) {
                IPCS_WriteLog("Asyn client run: read fail: %d", errno);
                break;
            }
        } else if (msgLen == 0) {
            continue;
        }

        result = IPCS_StreamToMsg(recvData, recvDataLen, &msg);

        result = threadArg->clientHook(&msg);
        if (result != IPCS_OK) {
            IPCS_WriteLog("Asyn client run: clientHook fail: %d", result);
        }
    }

    free(recvData);
    free(arg);

    return;
}

/* 异步调用 */
int IPCS_AsynCall(int fd, IPCS_Message *sendMsg)
{
    return 0;
}

/* 销毁客户端 */
int IPCS_DestroyClient(int fd)
{
    return 0;
}
