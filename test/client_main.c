/*
 * =====================================================================================
 *
 *       Filename:  client_main.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  12/28/2018 10:21:18 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dercury (Jim), dercury@qq.com
 *   Organization:  Perfect World
 *
 * =====================================================================================
 */

#include "test_main.h"
#include "ipcs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int UserSyncClient(void)
{
    int userSyncClientFd = 0;
	int result = 0;
	char *sendBuf = NULL;
	size_t sendBufLen = USER_INPUT_MAX_LEN;
	char *recvBuf = NULL;
	size_t recvBufLen = USER_INPUT_MAX_LEN;
    IPCS_Message sendMsg;
    IPCS_Message recvMsg;
	
	result = IPCS_CreateSyncClient(USER_SYNC_CLIENT_NAME, SYCN_SERVER_NAME, &userSyncClientFd);
    if (result != IPCS_OK) {
        TEST_PRINT("create user sync client fail: %d", result);
        return result;
    }
	
	sendBuf = malloc(sendBufLen);
	if (sendBuf == NULL) {
		TEST_PRINT("user sync client malloc fail.");
        return -1;
	}
    (void)memset(sendBuf, 0, sendBufLen);

	recvBuf = malloc(recvBufLen);
	if (recvBuf == NULL) {
		TEST_PRINT("user sync client malloc fail.");
        free(sendBuf);
        return -1;
	}
    (void)memset(recvBuf, 0, recvBufLen);

    printf("\r\nPlease input your request:\r\n");   /* for separate fgets input */
	while (fgets(sendBuf, sendBufLen, stdin) != NULL) {
        sendMsg.msgType = USER_SYNC_MSG;
        sendMsg.msgLen = strlen(sendBuf);
        sendMsg.msgValue = sendBuf;

        recvMsg.msgType = INVALID_MSG;
        recvMsg.msgLen = recvBufLen;
        recvMsg.msgValue = recvBuf;
        result = IPCS_ClientSyncCall(userSyncClientFd, &sendMsg, &recvMsg);
        if (result != IPCS_OK) {
		    TEST_PRINT("user client: %d sync call fail: %d.", userSyncClientFd, result);
            break;
        }

        if (recvMsg.msgType != sendMsg.msgType) {
		    TEST_PRINT("user client: %d sync call recv bad msg: %d.", userSyncClientFd, recvMsg.msgType);
        }
        fputs(recvBuf, stdout);

        (void)memset(sendBuf, 0, sendBufLen);
        (void)memset(recvBuf, 0, recvBufLen);
	}

    free(sendBuf);
    free(recvBuf);

    result = IPCS_DestroyClient(userSyncClientFd);
    if (result != IPCS_OK) {
        TEST_PRINT("destroy user sync client %d fail: %d", userSyncClientFd, result);
        return result;
    }
	
	return result;
}

int TimerSyncClient(int *timerSyncClientFd)
{
    int result = 0;
    int *fd = NULL;
    pthread_t threadId;

    fd = malloc(sizeof(int));
    if (fd == NULL) {
		TEST_PRINT("timer sync client malloc fail.");
        return -1;
    }

    do {
        *fd = 0;
        result = IPCS_CreateSyncClient(TIMER_SYNC_CLIENT_NAME, SYCN_SERVER_NAME, fd);
        if (result != IPCS_OK) {
            free(fd);
            TEST_PRINT("create timer sync client fail: %d", result);
            break;
        }
    
        result = IPCS_CreateThread(TimerSyncClientRun, fd, &threadId);
        if (result != IPCS_OK) {
            free(fd);
            TEST_PRINT("create timer sync client %d thread fail: %d", *fd, result);
            break;
        }

        *timerSyncClientFd = *fd;
    } while (0);

    return 0;
}

void *TimerSyncClientRun(void *arg)
{
    int result = 0;
    int *fd = (int *)arg;
    IPCS_Message sendMsg;
    static int sendMsgValue = 0;
    IPCS_Message recvMsg;
    int recvMsgValue = 0;

    for (; ; sendMsgValue++) {
        sendMsg.msgType = TIMER_SYNC_MSG;
        sendMsg.msgLen = sizeof(sendMsgValue);
        sendMsg.msgValue = &sendMsgValue;

        recvMsgValue = 0;
        recvMsg.msgType = INVALID_MSG;
        recvMsg.msgLen = sizeof(recvMsgValue);
        recvMsg.msgValue = &recvMsgValue;
        result = IPCS_ClientSyncCall(*fd, &sendMsg, &recvMsg);
        if (result != IPCS_OK) {
		    TEST_PRINT("timer client: %d sync call fail: %d.", *fd, result);
            break;
        }

        if ((recvMsg.msgType != sendMsg.msgType) || (recvMsgValue != sendMsgValue)) {
		    TEST_PRINT("timer client: %d sync call recv bad msg: type=%d, value=%d, expect msg: type=%d, value=%d.", 
                    *fd, recvMsg.msgType, recvMsgValue, sendMsg.msgType, sendMsgValue);
        } else {
            TEST_PRINT("timer client: %d sync call run %d times", *fd, sendMsgValue);
        }

        sleep(1);
    }

    free(arg);

    return NULL;
}

int UserAsynClientHook(IPCS_Message *msg)
{
    return 0;
}

int UserAsynClient(void)
{
    int userAsynClientFd = 0;
    return 0;
}

int TimerAsynClientHook(IPCS_Message *msg)
{
    return 0;
}

int TimerAsynClient(void)
{
    int timerAsynClientFd = 0;
    return 0;
}

int main(int argc, char **argv)
{
    int result = 0;
    int timerSyncClientFd = 0;

    result = TimerSyncClient(&timerSyncClientFd);
    if (result != IPCS_OK) {
        TEST_PRINT("timer sync client fail: %d", result);
        return result;
    }
    TEST_PRINT("timer sync client running ...");

    result = UserSyncClient();
    if (result != IPCS_OK) {
        TEST_PRINT("user sync client fail: %d", result);
        return result;
    }

    (void)fflush(NULL);

    pause();

    result = IPCS_DestroyClient(timerSyncClientFd);
    if (result != IPCS_OK) {
        TEST_PRINT("destroy timer sync client %d fail: %d", timerSyncClientFd, result);
    }
    (void)fflush(NULL);

    return result;
}


