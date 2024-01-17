/*
Dispatcher 类是一个抽象基类，定义了与事件处理相关的接口。
具体的事件处理类需要继承该基类并实现纯虚函数，以完成对事件的添加、删除、修改和监测等操作。
*/
#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include <string>
class EventLoop;
class Dispatcher
{
public:
    Dispatcher(EventLoop *evloop);
    virtual ~Dispatcher() = 0;
    // 添加
    virtual int add() = 0;
    // 删除
    virtual int remove() = 0;
    // 修改
    virtual int modify() = 0;
    // 事件监测
    virtual int dispatch(int timeout = 2) = 0; // 单位: s
    inline void setChannel(Channel *channel)
    {
        m_channel = channel;
    }

protected:
    std::string m_name = std::string();
    Channel *m_channel;
    EventLoop *m_evLoop;
};