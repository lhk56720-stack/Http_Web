/*
HttpResponse 类负责生成 HTTP 响应，包括状态行、响应头、空行以及具体的响应数据。
它提供了添加响应头的方法和一个用于发送数据的回调函数。
在 prepareMsg 函数中，将各部分数据按照 HTTP 响应的格式组织起来，并通过回调函数 sendDataFunc 发送给客户端。
*/
#pragma once
#include "Buffer.h"
#include <map>
#include <functional>
using namespace std;

// 定义状态码枚举
enum class StatusCode
{
    Unknown,
    OK = 200,
    MovedPermanently = 301,
    MovedTemporarily = 302,
    BadRequest = 400,
    NotFound = 404
};

// 定义结构体
class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();
    function<void(const string, struct Buffer *, int)> sendDataFunc;
    // 添加响应头
    void addHeader(const string key, const string value);
    // 组织http响应数据
    void prepareMsg(Buffer *sendBuf, int socket);
    inline void setFileName(string name)
    {
        m_fileName = name;
    }
    inline void setStatusCode(StatusCode code)
    {
        m_statusCode = code;
    }

private:
    // 状态行: 状态码, 状态描述
    StatusCode m_statusCode;
    string m_fileName;
    // 响应头 - 键值对
    map<string, string> m_headers;
    // 定义状态码和描述的对应关系
    const map<int, string> m_info = {
        {200, "OK"},
        {301, "MovedPermanently"},
        {302, "MovedTemporarily"},
        {400, "BadRequest"},
        {404, "NotFound"},
    };
};
