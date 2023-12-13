#pragma once
#include "Dispatcher.h"
#include "ChannelMap.h"
#include<pthread.h>
extern struct Dispatcher EpollDispatcher;
extern struct Dispatcher PollDispatcher;
extern struct Dispatcher SelectDispatcher;
enum ElementType//channel的处理方式
{
	ADD,
	DELETE,
	MODIFY
};
//定义任务队列的节点
struct ChannelElement {
	int type;	//channel的处理方式
	struct Channel* channel;
	struct ChannelElement* next;
};
struct EventLoop {
	bool isQuit;	//开关指定EventLoop是否在工作
	struct Dispatcher* dispatcher;
	void* dispatcherData;	//为了兼容不同模式的数据块
	//任务队列的首尾指针
	struct ChannelElement* head;
	struct ChannelElement* tail;
	//map
	struct ChannelMap* channelMap;
	//线程相关信息
	pthread_t threadID;
	char threadName[32]; 
	pthread_mutex_t mutex;
	int socketPair[2];		//存储本地通信的fd，通过socketPair初始化
};
//初始化
struct EventLoop* eventLoopInit();//子线程
struct EventLoop* eventLoopInitMain(const char* threadName);//主线程
//启动反应堆模型
int eventLoopRun(struct EventLoop* evLoop);
//处理被激活的文件描述符
int eventActivate(struct EventLoop* evLoop, int fd, int event);
//添加任务到任务队列
int eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type);
//处理任务队列中的任务
int eventLoopProcessTask(struct EventLoop* evLoop);
//处理dispatcher中的节点
int eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel);
int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel);
int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel);
