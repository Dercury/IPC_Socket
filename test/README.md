# 测试说明

2个服务端：SyncServer, AsynServer。

4个客户端：UserSyncClient, TimerSyncClient, UserAsynClient, TimerAsynClient。

UserSyncClient和TimerSyncClient是SyncServer的同步调用客户端；UserAsynClient和TimerAsynClient是AsynServer的异步调用客户端。

UserSyncClient从stdin接收输入，发送给SyncServer，SyncServer进行大小写翻转，然后发送给UserSyncClient，UserSyncClient输出到界面。

TimerSyncClient每隔1s发送1条带编号的消息（Type: Timer request, Value: 编号）给SyncServer，SyncServer解析后进行应答（Type: Timer response, Value: 编号），TimerSyncClient收到应答后，比较判断编号是否一致，如果不一致，则输出错误提示。

UserAsynClient从stdin接收输入，发送给AsynServer，AsynServer总是先sleep 5s，然后将输入拆分成单词，每个单词作为一条响应返回给UserAsynClient，UserAsynClient输出到界面。

TimerAsynClient与TimerSyncClient的行为一致，但AsynServer总是先sleep 5s，然后才进行应答。
