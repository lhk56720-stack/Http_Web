#include "TcpConnection.h"
int processRead(void* arg) {
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	//接收数据
	int ret=bufferSocketRead(conn->readBuf, conn->channel->fd);
	if (ret > 0) {
		//接收到了http请求，解析请求行
		int socket = conn->channel->fd;
		bool flag = parseHttpRequest(conn->request, conn->readBuf, conn->response, conn->writeBuf, socket);
		if (!flag) {
			//解析失败
			char* errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			bufferAppendData(conn->writeBuf, errMsg, strlen(errMsg));
		}
	}
	//断开连接
	eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
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
	conn->channel = channelInit(fd, ReadEvent, processRead, NULL, conn);
	eventLoopAddTask(evLoop, conn->channel, ADD);
	return conn;
}
