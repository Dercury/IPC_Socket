# IPC_Socket

/* 服务端响应的回调函数 */
typedef int (*ServerResponseFunc)(int fd, unsigned int cmdId, void *recvData, int recvDataLen);

/* 客户端响应的回调函数，仅用于异步调用时 */
typedef int (*ClientReponseFunc)(unsigned int cmdId, void *recvData, int recvDataLen);

/* 创建服务端，参数都是必须的入参 */
int IPCS_CreateServer(const char *serverName, ServerResponseFunc serverResponseProc);

/* 销毁服务端 */
int IPCS_DestroyServer(const char *serverName);

/* 服务端响应消息，服务端响应请求的回调函数中使用 */
int IPCS_RespondMessage(int fd, unsigned int cmdId, void *dataAddr, unsigned int dataLength);

/* 创建同步客户端 */
int IPCS_CreateSyncClient(const char *clientName, const char *serverName, int *fd);

/* 创建异步客户端 */
int IPCS_CreateAsynClient(const char *clientName, const char *serverName, ClientReponseFunc clientReponseProc, int *fd);

/* 销毁客户端 */
int IPCS_DestroyClient(int fd);

/* 同步调用 */
int IPCS_SyncCall(int fd, unsigned int cmdId, void *sendData, unsigned int sendDataLen, void *recvData, unsigned int *recvDataLen);

/* 异步调用 */
int IPCS_AsynCall(int fd, unsigned int cmdId, void *sendData, unsigned int sendDataLen);
