#include "Dispatcher.h"
#include <sys/select.h>
#include <stdio.h>
#define MAX 1024
struct SelectData
{
	fd_set readSet;
	fd_set writeSet;
};

static void *selectInit();
static void selectFdSet(struct Channel *channel, struct SelectData *data);
static void selectFdClr(struct Channel *channel, struct SelectData *data);
static int selectAdd(struct Channel *channel, struct EventLoop *evLoop);
static int selectRemove(struct Channel *channel, struct EventLoop *evLoop);
static int selectModify(struct Channel *channel, struct EventLoop *evLoop);
static int selectDispatcher(struct EventLoop *evLoop, int timeout);
static int selectClear(struct EventLoop *evLoop);

struct Dispatcher SelectDispatcher = {
	selectInit,
	selectAdd,
	selectRemove,
	selectModify,
	selectDispatcher,
	selectClear};

static void *selectInit()
{
	struct SelectData *data = (struct SelectData *)malloc(sizeof(struct SelectData));
	FD_ZERO(&data->readSet);
	FD_ZERO(&data->writeSet);
	return data;
}
static void selectFdSet(struct Channel *channel, struct SelectData *data)
{
	if (channel->events & ReadEvent)
	{
		FD_SET(channel->fd, &data->readSet);
	}
	if (channel->events & WriteEvent)
	{
		FD_SET(channel->fd, &data->writeSet);
	}
}
static void selectFdClr(struct Channel *channel, struct SelectData *data)
{
	if (channel->events & ReadEvent)
	{
		FD_CLR(channel->fd, &data->readSet);
	}
	if (channel->events & WriteEvent)
	{
		FD_CLR(channel->fd, &data->writeSet);
	}
}
static int selectAdd(struct Channel *channel, struct EventLoop *evLoop)
{
	struct SelectData *data = (struct SelectData *)evLoop->dispatcherData;
	if (channel->fd >= MAX)
	{
		printf("cross the border\n");
		return -1;
	}
	selectFdSet(channel, data);
	return 0;
}
static int selectRemove(struct Channel *channel, struct EventLoop *evLoop)
{
	struct SelectData *data = (struct SelectData *)evLoop->dispatcherData;
	if (channel->fd >= MAX)
	{
		printf("cross the border\n");
		return -1;
	}
	selectFdClr(channel, data);
	// 通过channel释放对应的TcpConnection资源
	channel->destroyCallback(channel->arg);
	return 0;
}
static int selectModify(struct Channel *channel, struct EventLoop *evLoop)
{
	struct SelectData *data = (struct SelectData *)evLoop->dispatcherData;
	if (channel->fd >= MAX)
	{
		printf("cross the border\n");
		return -1;
	}
	selectFdClr(channel, data);
	selectFdSet(channel, data);
	return 0;
}

static int selectDispatcher(struct EventLoop *evLoop, int timeout)
{
	struct SelectData *data = (struct SelectData *)evLoop->dispatcherData;
	struct timeval val;
	val.tv_sec = timeout;
	val.tv_usec = 0;
	// 对原始数据进行备份
	fd_set retmp = data->readSet;
	fd_set wrtmp = data->writeSet;
	int count = select(MAX, &retmp, &wrtmp, NULL, &val); // 时间单位:s
	if (count == -1)
	{
		perror("select");
		exit(0);
	}
	for (int i = 0; i < MAX; ++i)
	{
		if (FD_ISSET(i, &retmp))
		{
			eventActivate(evLoop, i, ReadEvent);
		}
		if (FD_ISSET(i, &wrtmp))
		{
			eventActivate(evLoop, i, WriteEvent);
		}
	}
	return 0;
}
static int selectClear(struct EventLoop *evLoop)
{
	struct SelectData *data = (struct SelectData *)evLoop->dispatcherData;
	free(data);
	return 0;
}