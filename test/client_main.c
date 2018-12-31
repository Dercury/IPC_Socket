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


int UserAsyncClientHook(IPCS_Message *msg)
{
    return 0;
}

int TimerAsynClientHook(IPCS_Message *msg)
{
    return 0;
}

int UserSyncClient(void)
{
    int userSyncClientFd = 0;
	int result = 0;
	char *sendBuf = NULL;
	size_t sendBufLen = MAXLINE;
	char *recvBuf = NULL;
	size_t recvBufLen = MAXLINE;
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

    printf("\r\n"); /* for separate fgets input */
	while (fgets(sendBuf, sendBufLen, stdin) != NULL) {
        sendMsg.msgType = USER_SYNC_MSG;
        sendMsg.msgLen = strlen(sendBuf);
        sendMsg.msgValue = sendBuf;

        recvMsg.msgType = USER_SYNC_MSG;
        recvMsg.msgLen = recvBufLen;
        recvMsg.msgValue = recvBuf;
        result = IPCS_ClientSyncCall(userSyncClientFd, &sendMsg, &recvMsg);
        if (result != IPCS_OK) {
		    TEST_PRINT("user client: %d sync call fail: %d.", userSyncClientFd, result);
            break;
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

int main(int argc, char **argv)
{
    int result = 0;
    int timerSyncClientFd = 0;
    int userAsynClientFd = 0;
    int timerAsynClientFd = 0;

//    result = IPCS_CreateSyncClient(TIMER_SYNC_CLIENT_NAME, SYCN_SERVER_NAME, &timerSyncClientFd);
//    if (result != IPCS_OK) {
//        TEST_PRINT("create timer sync client fail: %d", result);
//        return result;
//    }

    result = UserSyncClient();
    if (result != IPCS_OK) {
        TEST_PRINT("user sync client fail: %d", result);
        return result;
    }

    return 0;
}


