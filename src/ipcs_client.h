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

#ifndef __IPCS_CLIENT_H__
#define __IPCS_CLIENT_H__

#include "ipcs.h"
#include "ipcs_common.h"

/******************************************************************************/
typedef struct {
    int fd;
    ClientCallback clientHook;
} IPCS_AsynClientThreadArg;

/******************************************************************************/
int IPCS_CreateClientSocket(const char *clientName, const char *serverName, int *clientFd);

void *IPCS_AsynClientRun(void *arg);

/******************************************************************************/
int IPCS_AddSyncClientInfo(const char *clientName, const char *serverName, int fd);

int IPCS_AddAsynClientInfo(const char *clientName, const char *serverName, int fd, pthread_t pid, ClientCallback hook);

/******************************************************************************/

#endif /* __IPCS_CLIENT_H__ */

