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

/******************************************************************************/
/* 发送或接收消息的最大长度（字节数） */
#define IPCS_MESSAGE_MAX_LEN    (32*1024)

typedef struct {
    unsigned int msgType;
    unsigned int msgLen;
    void *msgValue;
} IPCS_Message;

/******************************************************************************/
typedef enum {
    IPCS_OK = 0,

    IPCS_MALLOC_FAIL,

    IPCS_SOCKET_FAIL,
    IPCS_BIND_FAIL,
    IPCS_LISTEN_FAIL,
    IPCS_ACCEPT_FAIL,
    IPCS_CONNECT_FAIL,

    IPCS_EPOLL_CREATE_FAIL,
    IPCS_EPOLL_CTL_FAIL,
    IPCS_EPOLL_WAIT_FAIL,
    IPCS_EPOLL_BAD_EVENT,

    IPCS_PTHREAD_ATTR_SET_FAIL,
    IPCS_PTHREAD_CREATE_FAIL,
    IPCS_PTHREAD_MUTEX_FAIL,

    IPCS_READ_FAIL,
    IPCS_WRITE_FAIL,

    IPCS_SERVER_HOOK_FAIL,
    IPCS_CLIENT_HOOK_FAIL,

    IPCS_MSG_TOO_LONG,  /* in */
    IPCS_BUF_TOO_SMALL, /* out */
    IPCS_STREAM_BUF_BAD,

    IPCS_UNREACHABLE,

    IPCS_ITEM_INFO_FULL,
    IPCS_NOT_FOUND,

    IPCS_PARAM_NULL,
    IPCS_PARAM_LEN,

    IPCS_ERROR_BUTT
} IPCS_ReturnValue;

/******************************************************************************/
/* 服务端响应的回调函数 */
typedef int (*ServerCallback)(int fd, IPCS_Message *msg);

/* 客户端响应的回调函数，仅用于异步调用时 */
typedef int (*ClientCallback)(IPCS_Message *msg);

/******************************************************************************/
/* 创建服务端，参数都是必须的入参 */
int IPCS_CreateServer(const char *serverName, ServerCallback serverHook);

/* 销毁服务端 */
int IPCS_DestroyServer(const char *serverName);

/* 服务端响应消息，服务端响应请求的回调函数中使用 */
int IPCS_ServerSendMessage(int fd, IPCS_Message *msg);

/******************************************************************************/
/* 创建同步客户端 */
int IPCS_CreateSyncClient(const char *clientName, const char *serverName, int *fd);

/* 创建异步客户端 */
int IPCS_CreateAsynClient(const char *clientName, const char *serverName, ClientCallback clientHook, int *fd);

/* 销毁客户端 */
int IPCS_DestroyClient(int fd);

/******************************************************************************/
/* 同步调用 */
int IPCS_ClientSyncCall(int fd, IPCS_Message *sendMsg, IPCS_Message *recvMsg);

/* 异步调用 */
int IPCS_ClientAsynCall(int fd, IPCS_Message *sendMsg);

/******************************************************************************/

#endif /* __IPCS_H__ */

