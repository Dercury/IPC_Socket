/*
 * =====================================================================================
 *
 *       Filename:  ipcs_common.h
 *
 *    Description:  IPC socket common
 *
 *        Version:  1.0
 *        Created:  12/28/2018 10:38:56 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dercury (Jim), dercury@qq.com
 *   Organization:  Perfect World
 *
 * =====================================================================================
 */

#ifndef __IPCS_COMMON_H__
#define __IPCS_COMMON_H__

#include "ipcs.h"
#include <pthread.h>

#define IPCS_SERVER_NAME_MAX_LEN     256
#define IPCS_CLIENT_NAME_MAX_LEN     256

typedef enum {
    IPCS_SERVER = 0,
    IPCS_SYNC_CLIENT,
    IPCS_ASYN_CLIENT
} IPCS_ItemType;

void IPCS_WriteLogImpl(const char *filename, unsigned int lineNum, const char *format, ...);

#define IPCS_WriteLog(format, ...)      IPCS_WriteLogImpl(__FILE__, __LINE__, (format), ##__VA_ARGS__)

int IPCS_CreateThread(void *(threadRunFunc)(void *), void *threadArg, pthread_t *threadId);

int IPCS_MsgToStream(IPCS_Message *msg, void *streamBuf, unsigned int *bufLen);

int IPCS_StreamToMsg(void *streamBuf, unsigned int bufLen, IPCS_Message *msg);

int IPCS_SendMessage(int fd, IPCS_Message *msg);

int IPCS_RecvSingleMsg(int fd, IPCS_Message *recvMsg);

int IPCS_RecvMultiMsg(int itemType, int fd, void *threadArg);

int IPCS_HandleRecvData(void *recvData, size_t recvDataLen, int itemType, int fd, void *threadArg);

int IPCS_ItemHandleMsg(int itemType, int fd, void *threadArg, IPCS_Message *msg);

#endif /* __IPCS_COMMON_H__ */

