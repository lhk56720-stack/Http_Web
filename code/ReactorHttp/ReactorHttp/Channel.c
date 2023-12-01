#include "Channel.h"

struct Channel* channelInit(int fd, int events, handleFunc readfunc, handleFunc writefun, void* arg)
{
	struct Channel* channel = (struct Channel*)malloc(sizeof(struct Channel));
	channel->fd = fd;
	channel->events = events;
	channel->readCallback = readfunc;
	channel->writeCallback = writefun;
	channel->arg = arg;
	return channel;
}

bool isWriteEventCheck(struct Channel* channel)
{
	return channel->events & WriteEvent;
}

void writeEventEnable(struct Channel* channel, bool flag)
{
	if (flag) {
		channel->events |= WriteEvent;
	}
	else
	{
		channel->events = channel->events & ~WriteEvent;
	}
}
