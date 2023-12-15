#include "HttpRequest.h"
#include<stdio.h>
#include<strings.h>
#define HeaderSize 16
struct HttpRequest* httpRequestInit()
{
	struct HttpRequest* req = (struct HttpRequest*)malloc(sizeof(struct HttpRequest));
	httpRequestReset(req);
	req->reqHeads = (struct HttpRequestHead*)malloc(sizeof(struct HttpRequestHead) * HeaderSize);
	return req;
}

void httpRequestReset(struct HttpRequest* req)
{
	req->method = NULL;
	req->url = NULL;
	req->version = NULL;
	req->reqHeadsNum = 0;
	req->curStatus = ParseReqLine;
}

void httpRequestResetEx(struct HttpRequest* req)
{
	free(req->method);
	free(req->url);
	free(req->version);
	if (req->reqHeads != NULL) {
		for (int i = 0; i < req->reqHeadsNum; ++i) {
			free(req->reqHeads[i].key);
			free(req->reqHeads[i].value);
		}
		free(req->reqHeads);
		req->reqHeads = NULL;
	}	
	httpRequestReset(req);
}

void httpRequestDestroy(struct HttpRequest* req)
{
	if (req != NULL) {
		httpRequestResetEx(req);
		free(req);
		req = NULL;
	}
}

enum HttpRequestStatus httpRequestStatus(struct HttpRequest* req)
{
	return req->curStatus;
}

void httpRequestAddHeader(struct HttpRequest* req, const char* key, const char* value)
{
	req->reqHeads[req->reqHeadsNum].key = key;
	req->reqHeads[req->reqHeadsNum].value = value;
	req->reqHeadsNum++;
}

char* httpRequestGetHeader(struct HttpRequest* req, const char* key)
{
	if (req != NULL) {
		for (int i = 0; i < req->reqHeadsNum; ++i) {
			if (strncasecmp(req->reqHeads->key, key, strlen(key)) == 0) {
				return req->reqHeads->value;
			}
		}
	}
	return NULL;
}

bool parseHttpRequestLine(struct HttpRequest* req, struct Buffer* readBuf)
{
	//读出请求行,保存字符串结束地址
	char* end = bufferFindLine(readBuf);
	//保存字符串起始地址
	char* start = readBuf->data + readBuf->readPos;
	//请求行总长度
	int lineSize = end - start;
	if (lineSize) {
		//请求方式	找到空格
		char* space = memmem(start, lineSize, " ", 1);
		int methodSize = space - start;
		req->method = (char*)malloc(methodSize + 1);	//给\0留空间
		strncpy(req->method, start, methodSize);
		req->method[methodSize] = '\0';

		//请求的静态资源	找到空格
		start = space + 1;
		space = memmem(start, end-start, " ", 1);
		int urlSize = space - start;
		req->url = (char*)malloc(urlSize + 1);	//给\0留空间
		strncpy(req->url, start, urlSize);
		req->method[urlSize] = '\0';

		//请求http版本
		start = space + 1;
		req->version = (char*)malloc(end - start + 1);	//给\0留空间
		strncpy(req->version, start, end - start);
		req->method[end - start] = '\0';

		//为解析请求头做准备
		readBuf->readPos += lineSize+2;
		//修改当前状态
		req->curStatus++;
		return true;
	}
	return false;
}
