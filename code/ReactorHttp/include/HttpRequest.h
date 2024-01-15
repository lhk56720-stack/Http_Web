#pragma once
#include <stdbool.h>
#include"Buffer.h"
#include"HttpResPonse.h"
//请求头键值对
struct HttpRequestHead
{
	char* key;
	char* value;
};
/*
Http请求格式：
	请求行
	请求头
	空行
	请求的数据块
*/
//当前的解析状态
enum HttpRequestStatus
{
	ParseReqLine,
	ParseReqHead,
	ParseReqBody,
	ParseReqDone
};	
//定义Http请求结构体
struct HttpRequest {
	char* method;
	char* url;
	char* version;
	struct HttpRequestHead* reqHeads;
	int reqHeadsNum;		//记录reqHeads有效键值对
	enum HttpRequestStatus curStatus;
};
//初始化
struct HttpRequest* httpRequestInit();
//资源重置
void httpRequestReset(struct HttpRequest* req);
void httpRequestResetEx(struct HttpRequest* req);
//资源销毁
void httpRequestDestroy(struct HttpRequest* req);
//获取处理状态
enum HttpRequestStatus httpRequestStatus(struct HttpRequest* req);
//添加请求头
void httpRequestAddHeader(struct HttpRequest* req, const char* key, const char* value);
//根据key获取value
char* httpRequestGetHeader(struct HttpRequest* req, const char* key);
//解析请求行
bool parseHttpRequestLine(struct HttpRequest* req, struct Buffer* readBuf);
//解析请求头
bool parseHttpRequestHeader(struct HttpRequest* req, struct Buffer* readBuf);
//解析http请求协议
bool parseHttpRequest(struct HttpRequest* req, struct Buffer* readBuf,struct HttpResponse* response,struct Buffer* sendBuf,int socket);
//处理http请求协议
bool processHttpRequest(struct HttpRequest* req, struct HttpResponse* response);



//发送文件
void sendFile(const char* fileName, struct Buffer* sendBuf, int cfd);
//发送目录
void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd);

//解码字符串
int hexToDec(char c);
void decodeMsg(char* to, char* from);
const char* getFileType(const char* name);