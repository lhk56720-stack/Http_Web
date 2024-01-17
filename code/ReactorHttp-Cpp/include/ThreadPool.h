/*
ThreadPool 类实现了一个简单的线程池，能够管理多个子线程，并从中取出一个子线程的反应堆实例。
线程池的启动会创建多个子线程，并每个子线程运行一个 WorkerThread 对象，实现了异步事件处理的多线程支持。
这有助于更好地处理并发任务和提高系统性能。
*/
#pragma once
#include "EventLoop.h"
#include <stdbool.h>
#include "WorkerThread.h"
#include <vector>
using namespace std;

// 定义线程池
class ThreadPool
{
public:
    ThreadPool(EventLoop *mainLoop, int count);
    ~ThreadPool();
    // 启动线程池
    void run();
    // 取出线程池中的某个子线程的反应堆实例
    EventLoop *takeWorkerEventLoop();

private:
    // 主线程的反应堆模型
    EventLoop *m_mainLoop;
    bool m_isStart;
    int m_threadNum;
    vector<WorkerThread *> m_workerThreads;
    int m_index;
};
