/*
WorkerThread 类负责管理子线程的生命周期，通过创建一个子线程，在该子线程中运行一个 EventLoop 对象，从而实现了异步事件处理。
该类的设计考虑了线程安全性，通过互斥锁和条件变量来保护共享资源。
 WorkerThread 对象的创建会阻塞主线程，等待 EventLoop 对象创建完成后才会继续执行。
*/
#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include "EventLoop.h"
using namespace std;

// 定义子线程对应的结构体
class WorkerThread
{
public:
    WorkerThread(int index);
    ~WorkerThread();
    // 启动线程
    void run();
    inline EventLoop* getEventLoop()
    {
        return m_evLoop;
    }

private:
    void running();

private:
    thread* m_thread;   // 保存线程的实例
    thread::id m_threadID; // ID
    string m_name;
    mutex m_mutex;  // 互斥锁
    condition_variable m_cond;    // 条件变量
    EventLoop* m_evLoop;   // 反应堆模型
};

