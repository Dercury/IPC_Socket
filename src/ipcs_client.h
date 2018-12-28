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

typedef struct {
    char clientName[IPCS_CLIENT_NAME_MAX_LEN];
    char serverName[IPCS_SERVER_NAME_MAX_LEN];
    int fd;
} IPCS_AsynClientThreadArg;

int IPCS_CreateClientSocket(const char *clientName, const char *serverName, int *clientFd);
void *IPCS_AsynClientRun(void *arg);

#endif /* __IPCS_CLIENT_H__ */

