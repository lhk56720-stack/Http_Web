#pragma once
//初始化监听
int initListenFd(unsigned short port);
//启动epoll
int epollRun(int lfd);
//与客户端建立连接
void* acceptClient(void* arg);
//接收Http请求
void* recvHttpRequest(void* arg);
//解析请求行
int parseRequestLine(const char* line, int cfd);
//发送文件
int sendFile(const char* fileName, int cfd);
//发送Http响应(状态行+响应头)
//状态行=状态码+状态描述
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length);
//发送目录
int sendDir(const char* dirName, int cfd);

const char* getFileType(const char* name);
int hexToDec(char c);
void decodeMsg(char* to, char* from);