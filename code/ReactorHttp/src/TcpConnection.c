#include "TcpConnection.h"
#include<stdio.h>
#include<string.h>
int processRead(void* arg) {
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	//接收数据
	int ret=bufferSocketRead(conn->readBuf, conn->channel->fd);
	printf("recv http request:%s\n", conn->readBuf->data + conn->readBuf->readPos);
	if (ret > 0) {
		//接收到了http请求，解析请求行
		int socket = conn->channel->fd;
#ifdef MSG_SEND_AUTO
		writeEventEnable(conn->channel, true);
		eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
#endif 
		bool flag = parseHttpRequest(conn->request, conn->readBuf, conn->response, conn->writeBuf, socket);
		if (!flag) {
			//解析失败
			char* errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			bufferAppendString(conn->writeBuf, errMsg);
		}
	}
#ifndef MSG_SEND_AUTO
	//断开连接
	eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
#endif
	return 0;
}
int processWrite(void* arg) {
	printf("start send data...\n");
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	//发送数据
	int count=bufferSendData(conn->writeBuf, conn->channel->fd);
	if (count > 0) {
		//判断数据是否完全被发送
		if (getBufferReadSize(conn->writeBuf) == 0) {
			//不再检测写事件
			writeEventEnable(conn->channel, false);
			eventLoopAddTask(conn->evLoop, conn->channel, MODIFY);
			eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
		}
	}
	return 0;
}
struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evLoop)
{
	struct TcpConnection* conn = (struct TcpConnection*)malloc(sizeof(struct TcpConnection));
	conn->evLoop = evLoop;
	conn->readBuf = bufferInit(10240);
	conn->writeBuf = bufferInit(10240);
	//http
	conn->request = httpRequestInit();
	conn->response = httpResponseInit();
	sprintf(conn->name, "TcpConnection-%d", fd);
	conn->channel = channelInit(fd, ReadEvent, processRead, processWrite, tcpConnectionDestroy, conn);
	eventLoopAddTask(evLoop, conn->channel, ADD);
	printf("connect client,threadName:%s,threadId:%s,connName:%s", evLoop->threadName, evLoop->threadID, conn->name);
	return conn;
}

int tcpConnectionDestroy(void* arg)
{
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	if (conn != NULL) {
		if (conn->readBuf && getBufferReadSize(conn->readBuf) == 0 &&
			conn->writeBuf && getBufferWriteSize(conn->writeBuf) == 0) {
			channelDestroy(conn->evLoop, conn->channel);
			bufferDestroy(conn->readBuf);
			bufferDestroy(conn->writeBuf);
			httpRequestDestroy(conn->request);
			httpResponseDestroy(conn->response);
			free(conn);
			conn = NULL;
		}
	}
	printf("connect over,connName:%s\n", conn->name);
	return 0;
}
