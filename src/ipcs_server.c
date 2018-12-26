#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>


#define MAXLINE 80

char *g_ServerSocketPath = "server.socket";
const int MAX_CLIENT_NUM = 20;

int main(int argc, char **argv)
{
    struct sockaddr_un serverAddr;
    struct sockaddr_un clientAddr;
	socklen_t clientAddrLen;
	int serverFd = 0;
    int clientFd = 0;
	char buf[MAXLINE] = {0};
    int msgLen = 0;
    int i;
    int result = 0;

    serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverFd < 0) {
        perror("socket error");
        exit(1);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_UNIX;
    strcpy(serverAddr.sun_path, g_ServerSocketPath);

    /* 如果调用bind时文件已存在，则bind返回错误，所以先删除文件 */
    unlink(g_ServerSocketPath);
    result = bind(serverFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (result < 0) {
        perror("bind error");
        exit(1);
    }
    printf("UNIX domain socket bound\n");

    result = listen(serverFd, MAX_CLIENT_NUM);
    if (result < 0) {
        perror("listen error");
        exit(1);
    }
    printf("Accepting connections ... \n");

    while (1) {
        clientAddrLen = sizeof(clientAddr);
        clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientFd < 0) {
            perror("accept error");
            continue;
        }

        while (1) {
            msgLen = read(clientFd, buf, sizeof(buf));
            if (msgLen < 0) {
                perror("read error");
                break;
            } else if (msgLen == 0) {
                printf("EOF\n");
                break;
            }

            printf("received: %s", buf);

            for (i = 0; i < msgLen; i++) {
                buf[i] = toupper(buf[i]);
            }

            write(clientFd, buf, msgLen);
        }

        close(clientFd);
    }

    close(serverFd);

    return 0;
}

