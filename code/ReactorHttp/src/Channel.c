#include "Channel.h"
#include <stdlib.h>
#include <stdio.h>
struct Channel *channelInit(int fd, int events, handleFunc readFunc, handleFunc writeFun, handleFunc destroyFunc, void *arg)
{
	struct Channel *channel = (struct Channel *)malloc(sizeof(struct Channel));
	channel->fd = fd;
	channel->events = events;
	channel->readCallback = readFunc;
	channel->writeCallback = writeFun;
	channel->destroyCallback = destroyFunc;
	channel->arg = arg;
	return channel;
}

bool isWriteEventCheck(struct Channel *channel)
{
	return channel->events & WriteEvent;
}

void writeEventEnable(struct Channel *channel, bool flag)
{
	if (flag)
	{
		channel->events |= WriteEvent;
	}
	else
	{
		channel->events = channel->events & ~WriteEvent;
	}
}
