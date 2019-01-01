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
#include <sys/un.h>

/******************************************************************************/
#define IPCS_ITEM_NAME_MAX_LEN      sizeof(((struct sockaddr_un *)0)->sun_path)

typedef enum {
    IPCS_SERVER = 0,
    IPCS_SYNC_CLIENT,
    IPCS_ASYN_CLIENT
} IPCS_ItemType;

/******************************************************************************/
void IPCS_WriteLogImpl(const char *filename, unsigned int lineNum, const char *format, ...);

#define IPCS_WriteLog(format, ...)      IPCS_WriteLogImpl(__FILE__, __LINE__, (format), ##__VA_ARGS__)

/******************************************************************************/
int IPCS_CreateThread(void *(threadRunFunc)(void *), void *threadArg, pthread_t *threadId);

/******************************************************************************/
int IPCS_MsgToStream(IPCS_Message *msg, void *streamBuf, unsigned int *bufLen);

int IPCS_StreamToMsg(void *streamBuf, unsigned int bufLen, IPCS_Message *msg);

/******************************************************************************/
int IPCS_SendMessage(int fd, IPCS_Message *msg);

int IPCS_RecvSingleMsg(int fd, IPCS_Message *recvMsg);

int IPCS_RecvMultiMsg(int itemType, int fd, void *threadArg);

int IPCS_HandleRecvData(void *recvData, size_t recvDataLen, int itemType, int fd, void *threadArg);

int IPCS_ItemHandleMsg(int itemType, int fd, void *threadArg, IPCS_Message *msg);

/******************************************************************************/
typedef struct {
    IPCS_ItemType type;
    char name[IPCS_ITEM_NAME_MAX_LEN];
    char peerName[IPCS_ITEM_NAME_MAX_LEN];
    int fd;
    int epollFd;
    pthread_t pid;
    void *hook;
} IPCS_ItemInfo;

int IPCS_AddItemsInfo(IPCS_ItemInfo *itemInfo);
int IPCS_FindItemsInfo(IPCS_ItemType type, const char *name, int fd, IPCS_ItemInfo *itemInfo);
int IPCS_IsItemExist(IPCS_ItemType type, const char *name, int fd);
int IPCS_DelItemsInfo(IPCS_ItemType type, const char *name, int fd);

/******************************************************************************/
#endif /* __IPCS_COMMON_H__ */

