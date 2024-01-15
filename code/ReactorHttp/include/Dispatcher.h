#pragma once
#include "Channel.h"
#include "EventLoop.h"
struct EventLoop;
//事件分发器，存放调用函数的指针
struct Dispatcher
{
	//初始化epoll/poll/select需要的数据块
	void* (*init)();
	//添加
	int (*add)(struct Channel* channel,struct EventLoop* evLoop);
	//删除
	int (*remove)(struct Channel* channel, struct EventLoop* evLoop);
	//修改
	int (*modify)(struct Channel* channel, struct EventLoop* evLoop);
	//检测
	int (*dispatcher)(struct EventLoop* evLoop, int timeout);
	//清除数据
	int (*clear)(struct EventLoop* evLoop);
};