/*
 * =====================================================================================
 *
 *       Filename:  ipcs_common.c
 *
 *    Description:  IPC socket
 *
 *        Version:  1.0
 *        Created:  12/28/2018 10:36:29 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dercury (Jim), dercury@qq.com
 *   Organization:  Perfect World
 *
 * =====================================================================================
 */

#include "ipcs_common.h"
#include "ipcs_server.h"
#include "ipcs_client.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
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
#define IPCS_LOG_LINE_MAX_LEN   1024
#define IPCS_PRINT_LOG_LINE     printf

void IPCS_WriteLogImpl(const char *filename, unsigned int lineNum, const char *format, ...)
{
    char *buf = NULL;
    size_t bufLen = IPCS_LOG_LINE_MAX_LEN;
    int result = 0;
    va_list ap;

    buf = (char *)malloc(bufLen);
    if (buf == NULL) {
        perror("malloc log buf fail");
        return;
    }
    (void)memset(buf, 0, bufLen);

    do {
        va_start(ap, format);
        result = vsnprintf(buf, bufLen, format, ap);
        va_end(ap);

        if (result <= 0) {
            perror("vsnprintf log fail");
            break;
        }

        result = IPCS_PRINT_LOG_LINE("\r\n[%s:%u]%s", filename, lineNum, buf);
        if (result <= 0) {
            perror("print log fail");
            break;
        }
    } while (0);

    free(buf);

    return;
}

/******************************************************************************/
int IPCS_CreateThread(void *(threadRunFunc)(void *), void *threadArg, pthread_t *threadId)
{
    pthread_attr_t threadAttr;
    int result = 0;

    result = pthread_attr_init(&threadAttr);
    if (result != 0) {
        perror("pthread attr init error");
        IPCS_WriteLog("Create thread: pthread attr init fail: %d", result);
        return IPCS_PTHREAD_ATTR_SET_FAIL;
    }

    /* 将threadAttr内相关属性设置为PTHREAD_CREATE_DETACHED，线程会变成unjoinable状态，
    * 则新线程不能用pthread_join来同步，且在退出时自行释放所占用的资源 */
    result = pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
    if (result != 0) {
        (void)pthread_attr_destroy(&threadAttr);
        perror("pthread attr set detach state error");
        IPCS_WriteLog("Create thread: pthread attr set detach state fail: %d", result);
        return IPCS_PTHREAD_ATTR_SET_FAIL;
    }

    result = pthread_create(threadId, &threadAttr, threadRunFunc, threadArg);
    if (result != 0) {
        (void)pthread_attr_destroy(&threadAttr);
        perror("pthread create error");
        IPCS_WriteLog("Create thread: pthread create fail: %d", result);
        return IPCS_PTHREAD_CREATE_FAIL;
    }

    result = pthread_attr_destroy(&threadAttr);
    if (result != 0) {
        (void)pthread_cancel(*threadId);
        perror("pthread attr destroy error");
        IPCS_WriteLog("Create thread: pthread attr destroy fail: %d", result);
        return IPCS_PTHREAD_ATTR_SET_FAIL;
    }

    return IPCS_OK;
}

/******************************************************************************/
int IPCS_MsgToStream(IPCS_Message *msg, void *streamBuf, unsigned int *bufLen)
{
    size_t msgHeaderLen = offsetof(IPCS_Message, msgValue);
    unsigned int msgLen = msgHeaderLen + msg->msgLen;

    if (msgHeaderLen > *bufLen) {
        return IPCS_STREAM_BUF_BAD;
    }

    if (msgLen > *bufLen) {
        return IPCS_MSG_TOO_LONG;
    }

    (void)memcpy(streamBuf, msg, msgHeaderLen);

    if (msg->msgLen > 0) {
        (void)memcpy((char *)streamBuf + msgHeaderLen, msg->msgValue, msg->msgLen);
    }

    *bufLen = msgLen;

    return IPCS_OK;
}

int IPCS_StreamToMsg(void *streamBuf, unsigned int bufLen, IPCS_Message *msg)
{
    size_t msgHeaderLen = offsetof(IPCS_Message, msgValue);

    if (bufLen < msgHeaderLen) {
        return IPCS_STREAM_BUF_BAD;
    }

    if (msg->msgLen < ((IPCS_Message *)streamBuf)->msgLen) {
        return IPCS_BUF_TOO_SMALL;
    }

    (void)memcpy(msg, streamBuf, msgHeaderLen);

    if (msg->msgLen != 0) {
        (void)memcpy(msg->msgValue, (char *)streamBuf + msgHeaderLen, msg->msgLen);
    }

    return IPCS_OK;
}

/******************************************************************************/
int IPCS_SendMessage(int fd, IPCS_Message *msg)
{
    unsigned int streamBufLen = IPCS_MESSAGE_MAX_LEN;
    void *streamBuf = NULL;
    int result = IPCS_OK;
    ssize_t writeLen = 0;

    streamBuf = malloc(streamBufLen);
    if (streamBuf == NULL) {
        perror("malloc error");
        IPCS_WriteLog("Send message: malloc fail, fd: %d.", fd);
        return IPCS_MALLOC_FAIL;
    }

    do {
        result = IPCS_MsgToStream(msg, streamBuf, &streamBufLen);
        if (result != IPCS_OK) {
            IPCS_WriteLog("Send message: msg to stream fail: %d, fd: %d.", result, fd);
            break;
        }

        writeLen = write(fd, streamBuf, streamBufLen);
        if (writeLen <= 0) {
            perror("write error");
            IPCS_WriteLog("Send message: write fd: %d fail: %d, errno: %d", fd, writeLen, errno);
            result = IPCS_WRITE_FAIL;
            break;
        }
    } while (0);

    free(streamBuf);

    return result;
}

int IPCS_RecvSingleMsg(int fd, IPCS_Message *recvMsg)
{
    int result = 0;
    void *streamBuf = NULL;
    unsigned int bufLen = IPCS_MESSAGE_MAX_LEN;
    ssize_t readLen = 0;

    streamBuf = malloc(bufLen);
    if (streamBuf == NULL) {
        perror("malloc error");
        IPCS_WriteLog("Fd: %d recv single msg: malloc fail.", fd);
        return IPCS_MALLOC_FAIL;
    }
    (void)memset(streamBuf, 0, bufLen);

    do {
        (void)memset(streamBuf, 0, bufLen);
        readLen = read(fd, streamBuf, bufLen);
        if (readLen <= 0) {
            perror("read error");
            IPCS_WriteLog("Fd: %d recv single msg: read fail: %d, errno: %d", fd, readLen, errno);
            result = IPCS_READ_FAIL;
            break;
        }

        result = IPCS_StreamToMsg(streamBuf, readLen, recvMsg);
        if (result != IPCS_OK) {
            IPCS_WriteLog("Fd: %d recv single msg: stream to msg fail: %d", fd, result);
            break;
        }
    } while (0);

    free(streamBuf);

    return result;
}

int IPCS_RecvMultiMsg(int itemType, int fd, void *threadArg)
{
    void *recvBuf = NULL;
    unsigned int recvBufLen = IPCS_MESSAGE_MAX_LEN;
    ssize_t recvLen = 0;
    int result = 0;

    recvBuf = malloc(recvBufLen);
    if (recvBuf == NULL) {
        perror("mallco error");
        IPCS_WriteLog("Fd: %d recv multi msg: malloc fail.", fd);
        return IPCS_MALLOC_FAIL;
    }

    do {
        (void)memset(recvBuf, 0, recvBufLen);
        recvLen = read(fd, recvBuf, recvBufLen);
        if (recvLen < 0) {
            if (errno != EAGAIN) {
                IPCS_WriteLog("Fd: %d recv multi msg: read fail: %d, errno: %d", fd, recvLen, errno);
                break;
            }
        } else if (recvLen == 0) {
            continue;
        }

        result = IPCS_HandleRecvData(recvBuf, recvLen, itemType, fd, threadArg);
        if (result != IPCS_OK) {
            IPCS_WriteLog("Client: %d recv multi msg: handle recv data fail: %d, len: %d", fd, result, recvLen);
            break;
        }
    } while (0);

    free(recvBuf);

    return result;
}

int IPCS_HandleRecvData(void *recvData, size_t recvDataLen, int itemType, int fd, void *threadArg)
{
    char *leftData = recvData;
    size_t leftDataLen = recvDataLen;
    size_t msgHeaderLen = offsetof(IPCS_Message, msgValue);
    int result = 0;
    IPCS_Message msg;
    unsigned int offset = 0;
    void *msgBuf = NULL;
    unsigned int msgBufLen = IPCS_MESSAGE_MAX_LEN;

    msgBuf = malloc(msgBufLen);
    if (msgBuf == NULL) {
        IPCS_WriteLog("Handle recv data: malloc fail.");
        return IPCS_MALLOC_FAIL;
    }

    while (leftDataLen > msgHeaderLen) {
        msg.msgType = 0;
        msg.msgLen = msgBufLen;
        msg.msgValue = msgBuf;

        result = IPCS_StreamToMsg(leftData, leftDataLen, &msg);
        if (result != IPCS_OK) {
            IPCS_WriteLog("Handle recv data: stream to msg fail: %d.", result);
            break;
        }

        result = IPCS_ItemHandleMsg(itemType, fd, threadArg, &msg);
        if (result != IPCS_OK) {
            break;
        }

        offset = msgHeaderLen + msg.msgLen;
        leftData += offset;
        leftDataLen -= offset;
    }

    free(msgBuf);

    return result;
}

int IPCS_ItemHandleMsg(int itemType, int fd, void *threadArg, IPCS_Message *msg)
{
    int result = IPCS_OK;

    switch (itemType) {
        case IPCS_SERVER:
            result = ((IPCS_ServerThreadArg *)threadArg)->serverHook(fd, msg);
            if (result != IPCS_OK) {
                IPCS_WriteLog("Server: %d handle message: server hook fail: %d.", fd, result);
                result = IPCS_SERVER_HOOK_FAIL;
            }
            break;
        case IPCS_SYNC_CLIENT:
            result = IPCS_UNREACHABLE;
            break;
        case IPCS_ASYN_CLIENT:
            result = ((IPCS_AsynClientThreadArg *)threadArg)->clientHook(msg);
            if (result != IPCS_OK) {
                IPCS_WriteLog("Asyn client: %d handle message: client hook fail: %d.", fd, result);
                result = IPCS_CLIENT_HOOK_FAIL;
            }
            break;
    }

    return result;
}

/******************************************************************************/
/** 
 * 增加全局变量保存信息的做法是不推荐的，因为它通常导致线程不安全、模块间耦合等问题。
 *
 * 但是这里引入全局变量保存服务端和客户端的信息，用于防止重复创建和销毁等。
 * 服务端和客户端的创建、同步调用、异步调用等都不依赖于该信息。
 * 如果能够保证不会重复创建，且不关心socket、线程等资源的释放，则可以不要该全局数据信息。
 **/
#define IPCS_ITEM_MAX_NUM       256
static IPCS_ItemInfo *g_IpcsItems = NULL;
static unsigned int g_IpcsItemsNum = 0;
static pthread_mutex_t g_IpcsItemsMutex = PTHREAD_MUTEX_INITIALIZER;

int IPCS_MallocImemsInfo(void)
{
    void *buf = NULL;
    size_t bufLen = sizeof(IPCS_ItemInfo) * IPCS_ITEM_MAX_NUM;

    buf = malloc(bufLen);
    if (buf == NULL) {
        perror("malloc error");
        IPCS_WriteLog("Malloc items info fail.");
        return IPCS_MALLOC_FAIL;
    }
    (void)memset(buf, 0, bufLen);

    g_IpcsItems = buf;

    return IPCS_OK;
}

int IPCS_CreateItemsInfo(void)
{
    int result = IPCS_OK;
    int mutexResult = 0;

    mutexResult = pthread_mutex_lock(&g_IpcsItemsMutex);
    if (mutexResult != 0) {
        perror("pthread_mutex_lock error");
        IPCS_WriteLog("Create items info: pthread_mutex_lock fail: %d, errno: %d.", mutexResult, errno);
        return IPCS_PTHREAD_MUTEX_FAIL;
    }

    do {
        if (g_IpcsItems != NULL) {
            break;
        }

        result = IPCS_MallocImemsInfo();
        g_IpcsItemsNum = 0;
    } while (0);

    mutexResult = pthread_mutex_unlock(&g_IpcsItemsMutex);
    if (mutexResult != 0) {
        perror("pthread_mutex_unlock error");
        IPCS_WriteLog("Create items info: pthread_mutex_unlock fail: %d, errno: %d.", mutexResult, errno);
        return IPCS_PTHREAD_MUTEX_FAIL;
    }

    return result;
}

void IPCS_FreeItemsInfo(void)
{
    if (g_IpcsItems != NULL) {
        free(g_IpcsItems);
        g_IpcsItems = NULL;
    }

    return;
}

int IPCS_DestroyItemsInfo(void)
{
    int mutexResult = 0;

    mutexResult = pthread_mutex_lock(&g_IpcsItemsMutex);
    if (mutexResult != 0) {
        perror("pthread_mutex_lock error");
        IPCS_WriteLog("Destroy items info: pthread_mutex_lock fail: %d, errno: %d.", mutexResult, errno);
        return IPCS_PTHREAD_MUTEX_FAIL;
    }

    IPCS_FreeItemsInfo();
    g_IpcsItemsNum = 0;

    mutexResult = pthread_mutex_unlock(&g_IpcsItemsMutex);
    if (mutexResult != 0) {
        perror("pthread_mutex_unlock error");
        IPCS_WriteLog("Destroy items info: pthread_mutex_unlock fail: %d, errno: %d.", mutexResult, errno);
        return IPCS_PTHREAD_MUTEX_FAIL;
    }

    return IPCS_OK;
}

int IPCS_AddItemAction(IPCS_ItemInfo *itemInfo)
{
    int loop = 0;
    int result = IPCS_UNREACHABLE;

    for (loop = 0; loop < IPCS_ITEM_MAX_NUM; loop++) {
        if (strlen(g_IpcsItems[loop].name) == 0) {
            g_IpcsItems[loop].type = itemInfo->type;
            (void)strcpy(g_IpcsItems[loop].name, itemInfo->name);
            (void)strcpy(g_IpcsItems[loop].peerName, itemInfo->peerName);
            g_IpcsItems[loop].fd = itemInfo->fd;
            g_IpcsItems[loop].epollFd = itemInfo->epollFd;
            g_IpcsItems[loop].pid = itemInfo->pid;
            g_IpcsItems[loop].hook = itemInfo->hook;

            g_IpcsItemsNum++;
            result = IPCS_OK;
            break;
        }
    }

    if (result != IPCS_OK) {
        IPCS_WriteLog("Add item action fail: %d.", result);
    }

    return result;
}

int IPCS_AddItemsInfo(IPCS_ItemInfo *itemInfo)
{
    int mutexResult = 0;
    int result = IPCS_UNREACHABLE;

    mutexResult = pthread_mutex_lock(&g_IpcsItemsMutex);
    if (mutexResult != 0) {
        perror("pthread_mutex_lock error");
        IPCS_WriteLog("Add items info: pthread_mutex_lock fail: %d, errno: %d.", mutexResult, errno);
        return IPCS_PTHREAD_MUTEX_FAIL;
    }

    do {
        if (g_IpcsItems == NULL) {
            result = IPCS_MallocImemsInfo();
            if (result != IPCS_OK) {
                break;
            }
            g_IpcsItemsNum = 0;
        }

        if (g_IpcsItemsNum >= IPCS_ITEM_MAX_NUM) {
            IPCS_WriteLog("Add items info: items num exceed: %d.", g_IpcsItemsNum);
            result = IPCS_ITEM_INFO_FULL;
            break;
        }

        result = IPCS_AddItemAction(itemInfo);
    } while (0);

    mutexResult = pthread_mutex_unlock(&g_IpcsItemsMutex);
    if (mutexResult != 0) {
        perror("pthread_mutex_unlock error");
        IPCS_WriteLog("Add items info: pthread_mutex_unlock fail: %d, errno: %d.", mutexResult, errno);
        return IPCS_PTHREAD_MUTEX_FAIL;
    }

    return result;
}

int IPCS_DelItemAction(int index)
{
    (void)memset(g_IpcsItems + index, 0, sizeof(IPCS_ItemInfo));
    g_IpcsItemsNum--;

    if (g_IpcsItemsNum == 0) {
        IPCS_FreeItemsInfo();
    }

    return IPCS_OK;
}

int IPCS_FindDelItemsInfo(IPCS_ItemType type, const char *name, const char *peerName, int isDel)
{
    int mutexResult = 0;
    int result = IPCS_NOT_FOUND;
    int loop = 0;

    mutexResult = pthread_mutex_lock(&g_IpcsItemsMutex);
    if (mutexResult != 0) {
        perror("pthread_mutex_lock error");
        IPCS_WriteLog("Del items info: pthread_mutex_lock fail: %d, errno: %d.", mutexResult, errno);
        return IPCS_PTHREAD_MUTEX_FAIL;
    }

    do {
        if ((g_IpcsItems == NULL) || (g_IpcsItemsNum == 0)) {
            result = IPCS_OK;
            break;
        }
    
        for (loop = 0; loop < IPCS_ITEM_MAX_NUM; loop++) {
            if ((g_IpcsItems[loop].type != type) ||
                (strcmp(g_IpcsItems[loop].name, name) != 0)) {
                continue;
            }

            if ((type != IPCS_SERVER) && 
                (strcmp(g_IpcsItems[loop].peerName, peerName) != 0)) {
                continue;
            }

            result = IPCS_OK;

            if (isDel) {
                result = IPCS_DelItemAction(loop);
            }

            break;
        }
    } while (0);

    mutexResult = pthread_mutex_unlock(&g_IpcsItemsMutex);
    if (mutexResult != 0) {
        perror("pthread_mutex_unlock error");
        IPCS_WriteLog("Del items info: pthread_mutex_unlock fail: %d, errno: %d.", mutexResult, errno);
        return IPCS_PTHREAD_MUTEX_FAIL;
    }

    return result;
}

int IPCS_FindItemsInfo(IPCS_ItemType type, const char *name, const char *peerName)
{
    return IPCS_FindDelItemsInfo(type, name, peerName, 0);
}

int IPCS_DelItemsInfo(IPCS_ItemType type, const char *name, const char *peerName)
{
    return IPCS_FindDelItemsInfo(type, name, peerName, 1);
}

/******************************************************************************/

