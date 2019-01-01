# IPC_Socket

```c

/* 发送或接收消息的最大长度（字节数） */
#define IPCS_MESSAGE_MAX_LEN    (32*1024)

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
int IPCS_ServerSendMessage(int fd, IPCS_Message *msg);

/* 创建同步客户端 */
int IPCS_CreateSyncClient(const char *clientName, const char *serverName, int *fd);

/* 创建异步客户端 */
int IPCS_CreateAsynClient(const char *clientName, const char *serverName, ClientCallback clientHook, int *fd);

/* 销毁客户端 */
int IPCS_DestroyClient(int fd);

/* 同步调用 */
int IPCS_ClientSyncCall(int fd, IPCS_Message *sendMsg, IPCS_Message *recvMsg);

/* 异步调用 */
int IPCS_ClientAsynCall(int fd, IPCS_Message *sendMsg);

```

# TODO

1.销毁客户端、服务端

2.入参检查

3.异步调用测试

