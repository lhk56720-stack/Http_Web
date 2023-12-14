#pragma once
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
	