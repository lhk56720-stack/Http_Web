#include "ThreadPool.h"
#include<assert.h>
#include"WorkThread.h"
struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int count)
{
	struct ThreadPool* pool = (struct ThreadPool*)malloc(sizeof(struct ThreadPool));
	pool->mainLoop = mainLoop;
	pool->isStart = false;
	pool->threadNum = count;
	pool->workThreads = (struct WorkThread*)malloc(sizeof(struct WorkThread) * (long unsigned int)count);
	pool->index = 0;
	return pool;
}

void threadPoolRun(struct ThreadPool* pool)
{
	assert(pool && !pool->isStart);
	if (pool->mainLoop->threadID != pthread_self())
		exit(0);
	pool->isStart = true;
	if (pool->threadNum) {
		for (int i = 0; i < pool->threadNum; ++i) {
			workThreadInit(&pool->workThreads[i], i);
			workThreadRun(&pool->workThreads[i]);
		}
	}
}

struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool)
{
	assert(pool->isStart);
	if (pool->mainLoop->threadID != pthread_self())
		exit(0);
	//从线程池中找到一个子线程，取出里面的反应堆实例
	struct EventLoop* evLoop = pool->mainLoop;
	if (pool->threadNum > 0) {
		evLoop = pool->workThreads[pool->index].evLoop;
		pool->index++;
		pool->index= pool->index % pool->threadNum;
	}
	return evLoop;
}
