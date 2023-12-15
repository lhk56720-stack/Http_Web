#include "HttpResPonse.h"
#include<strings.h>
#include<stdio.h>
#define HeadersSize 16
struct HttpResponse* httpResponseInit()
{
	struct HttpResponse* response = (struct HttpResponse*)malloc(sizeof(struct HttpResponse));
	response->headerNum = 0;
	int size = sizeof(struct ResponseHeader) * HeadersSize;
	response->headers = (struct ResponseHeader*)malloc(size);
	response->statusCode = Init;
	//初始化数组
	bzero(response->headers, size);
	bzero(response->StatusMsg, sizeof(response->StatusMsg));
	bzero(response->fileName, sizeof(response->fileName));
	//函数指针
	response->sendDataFunc = NULL;
	return response;
}

void httpResponseDestroy(struct HttpResponse* response)
{
	if (response != NULL) {
		free(response->headers);
		response->headers = NULL;
		free(response);
		response = NULL;
	}
}

void httpResponseAddHeader(struct HttpResponse* response, const char* key, const char* value)
{
	if (response == NULL || key == NULL || value == NULL)
		return;
	strcpy(response->headers[response->headerNum].key, key);
	strcpy(response->headers[response->headerNum].value, value);
	response->headerNum++;

}

void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int socket)
{
	//状态行
	char tmp[1024] = { 0 };
	sprintf(tmp, "HTTP/1.1 %d %s\r\n", response->statusCode, response->StatusMsg);
	bufferAppendData(sendBuf,tmp,strlen(tmp));
	//响应头
	for (int i = 0; i < response->headerNum; ++i) {
		sprintf(tmp, " % s: % s\r\n", response->headers[i].key, response->headers[i].value);
		bufferAppendData(sendBuf, tmp, strlen(tmp));
	}
	//空行
	bufferAppendData(sendBuf, "\r\n", 2);
	//回复的数据
	response->sendDataFunc(response->fileName, sendBuf, socket);
}
