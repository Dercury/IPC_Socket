/*
 * =====================================================================================
 *
 *       Filename:  ipcs.h
 *
 *    Description:  IPC socket (UNIX domain socket)
 *
 *        Version:  1.0
 *        Created:  12/28/2018 10:27:28 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dercury (Jim), dercury@qq.com
 *   Organization:  Perfect World
 *
 * =====================================================================================
 */

#ifndef __IPCS_H__
#define __IPCS_H__

#define IPCS_MESSAGE_MAX_LEN    (32*1024)

typedef enum {
    IPCS_OK = 0,
    IPCS_ERROR,
    IPCS_MALLOC_FAIL,
    IPCS_WRITE_FAIL,
    IPCS_READ_FAIL,
} IPCS_ReturnValue;

typedef struct {
    unsigned int msgType;
    unsigned int msgLen;
    void *msgValue;
} IPCS_Message;

/* 服务端响应的回调函数 */
typedef int (*ServerCallback)(int fd, IPCS_Message *msg);

/* 客户端响应的回调函数，仅用于异步调用时 */
typedef int (*ClientCallback)(IPCS_Message *msg);

/* 创建服务端，参数都是必须的入参 */
int IPCS_CreateServer(const char *serverName, ServerCallback serverHook);

/* 销毁服务端 */
int IPCS_DestroyServer(const char *serverName);

/* 服务端响应消息，服务端响应请求的回调函数中使用 */
int IPCS_RespondMessage(int fd, unsigned int cmdId, void *dataAddr, unsigned int dataLength);

/* 创建同步客户端 */
int IPCS_CreateSyncClient(const char *clientName, const char *serverName, int *fd);

/* 创建异步客户端 */
int IPCS_CreateAsynClient(const char *clientName, const char *serverName, ClientCallback clientHook, int *fd);

/* 销毁客户端 */
int IPCS_DestroyClient(int fd);

/* 同步调用 */
int IPCS_SyncCall(int fd, IPCS_Message *sendMsg, IPCS_Message *recvMsg);

/* 异步调用 */
int IPCS_AsynCall(int fd, IPCS_Message *sendMsg);


#endif /* __IPCS_H__ */
