#include "Dispatcher.h"
#include<sys/epoll.h>
#include<stdio.h>
#define MAX 512

static void* epollInit();
static int epollCtl(struct Channel* channel, struct EventLoop* evLoop, int op);	//模块化epoll_ctl
static int epollAdd(struct Channel* channel, struct EventLoop* evLoop);
static int epollRemove(struct Channel* channel, struct EventLoop* evLoop);
static int epollModify(struct Channel* channel, struct EventLoop* evLoop);
static int epollDispatcher(struct EventLoop* evLoop, int timeout);
static int epollClear(struct EventLoop* evLoop);

struct EpollData {
	int epfd;
	struct epoll_event* events;
};
struct Dispatcher EpollDispatcher = {
	epollInit,
	epollAdd,
	epollRemove,
	epollModify,
	epollDispatcher,
	epollClear
};

static void* epollInit() {
	struct EpollData* data = (struct EpollData*)malloc(sizeof(struct EpollData));
	data->epfd = epoll_create(1);
	if (data->epfd == -1) {
		perror("epoll_create");
		exit(0);
	}
	data->events = (struct epoll_event*)calloc(MAX, sizeof(struct epoll_event));
	return data;

}
int epollCtl(struct Channel* channel, struct EventLoop* evLoop, int op)
{
	//????????????????????????????????????????
	//data是epoll树的根节点
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	struct epoll_event ev;
	int events = 0;
	if (channel->events & ReadEvent) {
		events |= EPOLLIN;
	}
	if (channel->events & WriteEvent) {
		events |= EPOLLOUT;
	}
	ev.events = events;
	ev.data.fd = channel->fd;
	int ret = epoll_ctl(data->epfd, op, channel->fd, &ev);
	return ret;
}
static int epollAdd(struct Channel* channel, struct EventLoop* evLoop) {
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_ADD);
	if (ret == -1) {
		perror("epoll_add");
		exit(0);
	}
	return ret;
}
static int epollRemove(struct Channel* channel, struct EventLoop* evLoop) {
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_DEL);
	if (ret == -1) {
		perror("epoll_del");
		exit(0);
	}
	//通过channel释放对应的TcpConnection资源
	channel->destroyCallback(channel->arg);
	return ret;
}
static int epollModify(struct Channel* channel, struct EventLoop* evLoop) {
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_MOD);
	if (ret == -1) {
		perror("epoll_mod");
		exit(0);
	}
	return ret;
}
static int epollDispatcher(struct EventLoop* evLoop, int timeout) {
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	int count = epoll_wait(data->epfd, data->events, MAX, timeout * 1000);//时间单位:s
	for (int i = 0; i < count; ++i) {
		int events = data->events[i].events;
		int fd = data->events[i].data.fd;
		//客户端断开连接，删除fd
		
		continue;
		if (events & EPOLLIN) {
			eventActivate(evLoop, fd, ReadEvent);
		}
		if (events & EPOLLOUT) {
			eventActivate(evLoop, fd, WriteEvent);
		}
	}
	return 0;
}
static int epollClear(struct EventLoop* evLoop) {
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	free(data->events);
	close(data->epfd);
	free(data);
} 