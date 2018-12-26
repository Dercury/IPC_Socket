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
char *g_ClientSocketPath = "client.socket";

const int MAX_CLIENT_NUM = 20;

int main(int argc, char **argv)
{
    struct sockaddr_un serverAddr;
    struct sockaddr_un clientAddr;
    char buf[MAXLINE] = {0};
    int clientFd = 0;
    int result = 0;
    int msgLen = 0;

    clientFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (clientFd < 0) {
        perror("client socket error");
        exit(1);
    }

    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sun_family = AF_UNIX;
    strcpy(clientAddr.sun_path, g_ClientSocketPath);
    unlink(clientAddr.sun_path);
    result = bind(clientFd, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
    if (result < 0) {
        perror("client bind error");
        exit(1);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_UNIX;
    strcpy(serverAddr.sun_path, g_ServerSocketPath);
    unlink(serverAddr.sun_path);

    result = connect(clientFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (result < 0) {
        perror("client connect error");
        exit(1);
    }

    while (fgets(buf, MAXLINE, stdin) != NULL) {
        write(clientFd, buf, strlen(buf));

        msgLen = read(clientFd, buf, MAXLINE);
        if (msgLen < 0) {
            printf("the other side has been closed.\n");
        } else {
            write(STDOUT_FILENO, buf, msgLen);
        }
    }

    close(clientFd);

    return 0;
}

