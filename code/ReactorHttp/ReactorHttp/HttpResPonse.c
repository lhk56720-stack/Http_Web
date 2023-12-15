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
