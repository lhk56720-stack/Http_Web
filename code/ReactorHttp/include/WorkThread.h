#pragma once
#include <pthread.h>
#include"EventLoop.h"
//定义子线程对应的结构体
struct WorkThread {
	pthread_t threadID;
	char name[32];
	pthread_mutex_t mutex;	//互斥锁
	pthread_cond_t cond;	//条件变量
	struct EventLoop* evLoop;	//反应堆模型
};
//初始化
int workThreadInit(struct WorkThread* thread, int index);
//启动线程
void workThreadRun(struct WorkThread* thread);