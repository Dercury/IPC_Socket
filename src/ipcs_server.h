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

/******************************************************************************/
#define MAX_CLIENT_NUM      20

#define EPOLL_SIZE          20
#define EPOLL_RUN_TIMEOUT   -1

/******************************************************************************/
typedef struct {
    char name[IPCS_ITEM_NAME_MAX_LEN];
    ServerCallback serverHook;
} IPCS_ServerThreadArg;

/******************************************************************************/
int IPCS_CheckCreatingServer(const char *serverName, ServerCallback serverHook);

void *IPCS_ServerRun(void *arg);

int IPCS_CreateServerSocket(const char *serverName, int *serverFd);

int IPCS_CreateServerEpoll(int serverFd, int *epollFd);

int IPCS_HandleServerEpollEvents(int serverFd, int epollFd, IPCS_ServerThreadArg *threadArg);

int IPCS_ServerAcceptClient(int serverFd, int epollFd);

int IPCS_ServerHandleMessage(int clientFd, IPCS_ServerThreadArg *threadArg);

/******************************************************************************/
int IPCS_AddServerInfo(const char *serverName, int fd, int epollFd, pthread_t pid, ServerCallback hook);

/******************************************************************************/

#endif /* __IPCS_SERVER_H__ */

