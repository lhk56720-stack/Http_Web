/*
EpollDispatcher 类是针对 Linux 平台的事件处理器，使用 epoll 实现了事件的添加、删除、修改和监测。
它继承自 Dispatcher 类，通过实现 Dispatcher 中定义的虚函数来完成具体的事件处理逻辑。
*/
#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <string>
#include <sys/epoll.h>
using namespace std;

class EpollDispatcher : public Dispatcher
{
public:
    EpollDispatcher(EventLoop *evloop);
    ~EpollDispatcher() override;
    // 添加
    int add() override;
    // 删除
    int remove() override;
    // 修改
    int modify() override;
    // 事件监测
    int dispatch(int timeout = 2) override; // 单位: s

private:
    int epollCtl(int op);

private:
    int m_epfd;
    struct epoll_event *m_events;
    const int m_maxNode = 520;
};