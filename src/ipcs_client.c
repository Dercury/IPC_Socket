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
int IPCS_SyncCall(int fd, unsigned int cmdId, void *sendData, unsigned int sendDataLen, void *recvData, unsigned int *recvDataLen)
{
    return 0;
}

/* 异步调用 */
int IPCS_AsynCall(int fd, unsigned int cmdId, void *sendData, unsigned int sendDataLen)
{
    return 0;
}

/* 创建异步客户端 */
int IPCS_CreateAsynClient(const char *clientName, const char *serverName, ClientReponseFunc clientReponseProc, int *fd)
{
    pthread_t threadId;
    pthread_attr_t threadAttr;
    IPCS_AsynClientThreadArg *threadArg = NULL;
    int result = 0;
    
    result = IPCS_CreateClientSocket(clientName, serverName, fd);

    threadArg = (IPCS_AsynClientThreadArg *)malloc(sizeof(IPCS_AsynClientThreadArg));
    if (threadArg == NULL) {
        return -1;
    }
    (void)memset(threadArg, 0, sizeof(IPCS_AsynClientThreadArg));

    /* 将threadAttr内相关属性设置为PTHREAD_CREATE_DETACHED，线程会变成unjoinable状态，
     * 则新线程不能用pthread_join来同步，且在退出时自行释放所占用的资源 */
    pthread_attr_init(&threadAttr);
    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);

    //snprintf(threadArg->name, sizeof(threadArg->name), "%s", serverName);
    //threadArg->responseProc = serverResponseProc;
    pthread_create(&threadId, &threadAttr, IPCS_AsynClientRun, threadArg);

    pthread_attr_destroy(&threadAttr);

    return 0;
}

void *IPCS_AsynClientRun(void *arg)
{
    IPCS_AsynClientThreadArg *threadArg = (IPCS_AsynClientThreadArg *)arg;
	int serverFd = 0;


    free(arg);

    return;
}

/* 销毁客户端 */
int IPCS_DestroyClient(int fd)
{
    return 0;
}

