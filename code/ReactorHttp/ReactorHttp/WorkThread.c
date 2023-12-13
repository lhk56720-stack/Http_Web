#include "WorkThread.h"

int workThreadInit(struct WorkThread* thread, int index)
{
	thread->evLoop = NULL;
	thread->threadID = 0;
	sprintf(thread->name, "SubThread-%d", index);
	pthread_mutex_init(&thread->mutex,NULL);
	pthread_cond_init(&thread->cond, NULL);
	return 0;
}
//子线程的回调函数
void* subThreadRunning(void* arg) {
	struct WorkThread* thread = (struct WorkThread*)arg;
	pthread_mutex_lock(&thread->mutex);
	thread->evLoop = eventLoopInitMain(thread->name);
	pthread_mutex_unlock(&thread->mutex);
	pthread_cond_signal(&thread->cond);
	eventLoopRun(thread->evLoop);
	return NULL;
}
void workThreadRun(struct WorkThread* thread)
{
	//创建子线程
	pthread_create(&thread->threadID, NULL, subThreadRunning, thread);
	//阻塞主线程，等待子线程的eventLoop初始化
	pthread_mutex_lock(&thread->mutex);
	while (thread->evLoop == NULL) {
		pthread_cond_wait(&thread->cond, &thread->mutex);	
	}
	pthread_mutex_unlock(&thread->mutex);
}
