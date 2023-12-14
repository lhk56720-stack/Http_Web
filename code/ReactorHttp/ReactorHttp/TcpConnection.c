#include "TcpConnection.h"

struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evLoop)
{
	struct TcpConnection* conn = (struct TcpConnection*)malloc(sizeof(struct TcpConnection));
	conn->evLoop = evLoop;
	conn->readBuf = bufferInit(10240);
	conn->writeBuf = bufferInit(10240);
	sprintf(conn->name, "TcpConnection-%d", fd);
	conn->channel = channelInit(fd, ReadEvent, processRead, NULL, NULL);
	eventLoopAddTask(evLoop, conn->channel, ADD);
	return conn;
}
