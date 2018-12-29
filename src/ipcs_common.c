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

#define IPCS_LOG_MAX_LEN    1024
#define IPCS_LOG_PRINT      printf

void IPCS_WriteLogImpl(const char *filename, unsigned int lineNum, const char *format, ...)
{
    char *buf = NULL;
    size_t bufLen = IPCS_LOG_MAX_LEN;
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

        result = IPCS_LOG_PRINT("\r\n[%s:%u]%s", filename, lineNum, buf);
        if (result <= 0) {
            perror("print log fail");
            break;
        }
    } while (0);

    free(buf);

    return;
}

int IPCS_CreateThread(void *(threadRunFunc)(void *), void *threadArg, pthread_t *threadId)
{
    pthread_attr_t threadAttr;

    /* 将threadAttr内相关属性设置为PTHREAD_CREATE_DETACHED，线程会变成unjoinable状态，
    * 则新线程不能用pthread_join来同步，且在退出时自行释放所占用的资源 */
    pthread_attr_init(&threadAttr);
    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);

    pthread_create(threadId, &threadAttr, threadRunFunc, threadArg);

    pthread_attr_destroy(&threadAttr);

    return 0;
}

int IPCS_MsgToStream(IPCS_Message *msg, void *streamBuf, unsigned int *bufLen)
{
    return 0;
}

int IPCS_StreamToMsg(void *streamBuf, unsigned int bufLen, IPCS_Message *msg)
{
    return 0;
}
