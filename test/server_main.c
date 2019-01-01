/*
 * =====================================================================================
 *
 *       Filename:  server_main.c
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int SyncServerHook(int fd, IPCS_Message *msg)
{
    char *ptr = NULL;
    int result = IPCS_OK;

    if (msg->msgType == USER_SYNC_MSG) {
        for (ptr = msg->msgValue; (ptr != NULL) && (*ptr != '\0'); ptr++) {
            if (isupper(*ptr)) {
                *ptr = tolower(*ptr);
            } else if (islower(*ptr)) {
                *ptr = toupper(*ptr);
            }
        }

        result = IPCS_ServerSendMessage(fd, msg);
        if (result != IPCS_OK) {
            TEST_PRINT("Server send msg to user sync client: %d fail: %d", fd, result);
            return result;
        }
    } else if (msg->msgType == TIMER_SYNC_MSG) {
        result = IPCS_ServerSendMessage(fd, msg);
        if (result != IPCS_OK) {
            TEST_PRINT("Server send msg to timer sync client: %d fail: %d", fd, result);
            return result;
        }
    }

    return result;
}

int AsynServerHook(int fd, IPCS_Message *msg)
{
    return 0;
}

int main(int argc, char **argv)
{
    int result = 0;

    result = IPCS_CreateServer(SYCN_SERVER_NAME, SyncServerHook);
    if (result != IPCS_OK) {
        TEST_PRINT("create sync server fail: %d", result);
        return result;
    }

    result = IPCS_CreateServer(ASYC_SERVER_NAME, AsynServerHook);
    if (result != IPCS_OK) {
        TEST_PRINT("create asyn server fail: %d", result);
        return result;
    }

    (void)fflush(NULL);

    return pause();
}

