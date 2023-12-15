#include "EventLoop.h"
#include<string.h>
#include<assert.h>
#include<sys/socket.h>
struct EventLoop* eventLoopInit()
{
	return eventLoopInitMain(NULL);
}
//写数据用于唤醒线程
void taskWakeup(struct EventLoop* evLoop) {
	const char* msg = "wake up";
	write(evLoop->socketPair[0], msg, strlen(msg));
}
//读数据唤醒线程
int readLocalMessage(void* arg) {
	struct EventLoop* evLoop = (struct EventLoop*)arg;
	char buf[32];
	read(evLoop->socketPair[1], buf, sizeof(buf));
	return 0;
}
struct EventLoop* eventLoopInitMain(const char* threadName)
{
	struct EventLoop* evLoop = (struct EventLoop*)malloc(sizeof(struct EventLoop));
	evLoop->isQuit = false;
	evLoop->threadID = pthread_self();
	pthread_mutex_init(&evLoop->mutex, NULL);
	if (threadName == NULL)
		strcpy(evLoop->threadName, "MainThread");
	else
		strcpy(evLoop->threadName, threadName);
	evLoop->dispatcher = &EpollDispatcher;
	evLoop->dispatcherData = evLoop->dispatcher->init();
	evLoop->head = evLoop->tail = NULL;
	evLoop->channelMap = channelMapInit(128);

	int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, evLoop->socketPair);
	if (ret == -1) {
		perror("sockpair");
		exit(0);
	}
	//指定socketPair[0]发送数据		socketPair[1]接收数据
	struct Channel* channel = channelInit(evLoop->socketPair[1], ReadEvent, readLocalMessage, NULL, NULL, evLoop);
	eventLoopAddTask(evLoop, channel, ADD);
	return evLoop;
}

int eventLoopRun(struct EventLoop* evLoop)
{
	assert(evLoop != NULL);
	//取出事件分发和检测模型
	struct Dispatcher* dispatcher = evLoop->dispatcher;
	//比较线程ID是否正常
	if (evLoop->threadID != pthread_self()) {
		perror("threadID");
		return -1;
	}
	//循环进行事件处理
	while (!evLoop->isQuit) {
		dispatcher->dispatcher(evLoop, 2);
		eventLoopProcessTask(evLoop);
	}
	return 0;
}

int eventActivate(struct EventLoop* evLoop, int fd, int event)
{
	if (evLoop == NULL || fd < 0) {
		return -1;
	}
	//取出channel
	struct Channel* channel = evLoop->channelMap->list[fd];
	assert(channel->fd == fd);
	if (event & ReadEvent && channel->readCallback) {
		channel->readCallback(channel->arg);
	 }
	if (event & WriteEvent && channel->writeCallback) {
		channel->writeCallback(channel->arg);
	}

	return 0;
}

int eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type)
{
	//加锁保护共享资源
	pthread_mutex_lock(&evLoop->mutex);
	//创建新结点
	struct ChannelElement* node = (struct ChannelElement*)malloc(sizeof(struct ChannelElement));
	node->channel = channel;
	node->type = type;
	node->next = NULL;
	//链表为空
	if (evLoop->head == NULL) {
		evLoop->head = evLoop->tail = node;
	}
	else {
		evLoop->tail->next = node;
		evLoop->tail = node;
	}
	pthread_mutex_unlock(&evLoop->mutex);
	//处理节点
	/*
		添加任务节点的操作由主线程发起，但不能让主线程处理任务队列，需要由当前的子线程去处理
		路径1：子线程添加任务子线程自己处理
		路径2：主线程添加任务唤醒子线程，在唤醒子线程后子线程调用处理函数数量任务
	*/
	if (pthread_self() == evLoop->threadID) {
		//当前为子线程
		eventLoopProcessTask(evLoop);
	}
	else {
		//当前为主线程
		taskWakeup(evLoop);
	}


	return 0;
}

int eventLoopProcessTask(struct EventLoop* evLoop)
{
	pthread_mutex_lock(&evLoop->mutex);
	//取出头结点
	struct ChannelElement* head = evLoop->head;
	while (head != NULL) {
		struct Channel* channel = head->channel;
		switch (head->type) {
			case ADD:
				eventLoopAdd(evLoop, channel);
				break;
			case DELETE:
				eventLoopRemove(evLoop, channel);
				break;
			case MODIFY:
				eventLoopModify(evLoop, channel);
				break;
		}
		struct ChannelElement* tmp = head;
		head = head->next;
		free(tmp);
		tmp->next = NULL;
	}
	evLoop->head = evLoop->tail = NULL;
	pthread_mutex_unlock(&evLoop->mutex);
	return 0;
}

int eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel)
{
	int fd = channel->fd;
	struct ChannelMap* channelMap = evLoop->channelMap;
	if (fd >= channelMap->size) {
		//扩容
		if (!makeMapRoom(channelMap, fd, sizeof(struct ChannelMap*))) {
			perror("makeMapRoom");
			exit(0);
		}
	}
	if (channelMap->list[fd] == NULL) {
		channelMap->list[fd] = channel;
		evLoop->dispatcher->add(channel, evLoop);
	}
	return 0;
}

int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel)
{
	int fd = channel->fd;
	struct ChannelMap* channelMap = evLoop->channelMap;
	if (fd >= channelMap->size)
		return -1;
	int ret = evLoop->dispatcher->remove(channel, evLoop);
	return ret;
}

int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel)
{
	int fd = channel->fd;
	struct ChannelMap* channelMap = evLoop->channelMap;
	if (fd >= channelMap->size||channelMap->list[fd]==NULL)
		return -1;
	int ret = evLoop->dispatcher->modify(channel, evLoop);
	return ret;
}

int channelDestroy(struct EventLoop* evLoop, struct Channel* channel)
{
	//删除channel和fd的对应关系
	evLoop->channelMap->list[channel->fd] = NULL;
	//关闭fd
	close(channel->fd);
	//释放channel
	free(channel);
	channel= NULL;
	return 0;
}
