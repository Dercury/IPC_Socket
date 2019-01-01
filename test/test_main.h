/*
 * =====================================================================================
 *
 *       Filename:  test_main.h
 *
 *    Description:  test main
 *
 *        Version:  1.0
 *        Created:  12/31/2018 03:07:28 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dercury (Jim), dercury@qq.com
 *   Organization:  Perfect World
 *
 * =====================================================================================
 */

#ifndef __TEST_MAIN_H__
#define __TEST_MAIN_H__

#include <pthread.h>
#include <stdio.h>

#define TEST_PRINT(format, ...)     (void)printf("\r\n<%s:%u>" format, __FILE__, __LINE__, ##__VA_ARGS__)

#define SYCN_SERVER_NAME            "/usr/tmp/sync_server"
#define ASYC_SERVER_NAME            "/usr/tmp/asyn_server"

#define USER_SYNC_CLIENT_NAME       "/usr/tmp/user_sync_client"
#define TIMER_SYNC_CLIENT_NAME      "/usr/tmp/timer_sync_client"
#define USER_ASYN_CLIENT_NAME       "/usr/tmp/user_asyn_client"
#define TIMER_ASYN_CLIENT_NAME      "/usr/tmp/timer_asyn_client"

#define USER_INPUT_MAX_LEN     256

typedef enum {
    INVALID_MSG = 0,
    USER_SYNC_MSG,
    TIMER_SYNC_MSG,
    USER_ASYN_MSG,
    TIMER_ASYN_MSG
} TestMsgType;

void *TimerSyncClientRun(void *arg);

int IPCS_CreateThread(void *(threadRunFunc)(void *), void *threadArg, pthread_t *threadId);

#endif /* __TEST_MAIN_H__ */

