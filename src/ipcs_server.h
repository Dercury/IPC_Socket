/*
 * =====================================================================================
 *
 *       Filename:  ipcs_server.h
 *
 *    Description:  IPC socket server
 *
 *        Version:  1.0
 *        Created:  12/28/2018 10:35:12 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dercury (Jim), dercury@qq.com
 *   Organization:  Perfect World
 *
 * =====================================================================================
 */

#ifndef __IPCS_SERVER_H__
#define __IPCS_SERVER_H__

#include "ipcs.h"
#include "ipcs_common.h"

#define MAX_CLIENT_NUM  20

#define EPOLL_SIZE 20
#define EPOLL_RUN_TIMEOUT   500

typedef struct {
    char name[IPCS_SERVER_NAME_MAX_LEN];
    ServerCallback serverHook;
} IPCS_ServerThreadArg;

void *IPCS_ServerRun(void *arg);

int IPCS_CreateServerSocket(const char *serverName, int *serverFd);

int IPCS_CreateServerEpoll(int serverFd, int *epollFd);

int IPCS_HandleServerEpollEvents(int serverFd, int epollFd, IPCS_ServerThreadArg *threadArg);

int IPCS_AcceptClientEpoll(int serverFd, int epollFd);

int IPCS_HandleMessage(int clientFd, IPCS_ServerThreadArg *threadArg);

#endif /* __IPCS_SERVER_H__ */
