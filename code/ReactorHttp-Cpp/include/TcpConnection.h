/*
TcpConnection 类封装了一个 TCP 连接的相关逻辑，包括读写缓冲区的管理、HTTP 请求的解析、HTTP 响应的生成以及事件回调函数。
该类与事件循环协同工作，通过事件回调函数处理连接上的读写事件，并在适当的时机关闭连接。
*/
#pragma once
#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

//#define MSG_SEND_AUTO

class TcpConnection
{
public:
    TcpConnection(int fd, EventLoop* evloop);
    ~TcpConnection();

    static int processRead(void* arg);
    static int processWrite(void* arg);
    static int destroy(void* arg);
private:
    string m_name;
    EventLoop* m_evLoop;
    Channel* m_channel;
    Buffer* m_readBuf;
    Buffer* m_writeBuf;
    // http 协议
    HttpRequest* m_request;
    HttpResponse* m_response;
};