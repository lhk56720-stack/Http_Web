/*
TcpServer 类封装了一个简单的TCP服务器的实现，通过多线程和反应堆模型，能够同时处理多个客户端连接。
通过 TcpConnection 类处理连接，实现了基本的事件循环和任务处理。
*/
#pragma once
#include "EventLoop.h"
#include "ThreadPool.h"

class TcpServer
{
public:
    TcpServer(unsigned short port, int threadNum);
    // 初始化监听
    void setListen();
    // 启动服务器
    void run();
    static int acceptConnection(void* arg);

private:
    int m_threadNum;
    EventLoop* m_mainLoop;
    ThreadPool* m_threadPool;
    int m_lfd;
    unsigned short m_port;
};

