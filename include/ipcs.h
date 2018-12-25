
int IPCS_CreateServer(const char *serverName, int (*ServerResponseProc)(int fd, unsigned int cmdId, void *recvData, int recvDataLen));

int IPCS_DestroyServer(const char *serverName);

unsigned int IPCS_RespondMessage(int fd, unsigned int cmdId, void *dataAddr, unsigned int dataLength);



unsigned int IPCS_CreateSyncClient(const char *clientName, const char *serverName, int *fd);

unsigned int IPCS_CreateAsynClient(const char *clientName, const char *serverName, unsigned int (*ClientReponseProc)(unsigned int cmdId, void *recvData, int recvDataLen), int *fd);

int IPCS_DestroyClient(int fd);



int IPCS_SyncCall(int fd, unsigned int cmdId, void *sendData, unsigned int sendDataLen, unsigned int *recvCmdId, void **recvData, unsigned int *recvDataLen);

int IPCS_AsynCall(int fd, unsigned int cmdId, void *sendData, unsigned int sendDataLen);
