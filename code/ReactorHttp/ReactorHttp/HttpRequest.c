#include "HttpRequest.h"
#include<stdio.h>
#define HeaderSize 16
struct HttpRequest* httpRequestInit()
{
	struct HttpRequest* req = (struct HttpRequest*)malloc(sizeof(struct HttpRequest));
	req->method = NULL;
	req->url = NULL;
	req->version = NULL;
	req->reqHeads = (struct HttpRequestHead*)malloc(sizeof(struct HttpRequestHead) * HeaderSize);
	req->reqHeadsNum = 0;
	req->curStatus = ParseReqLine;
	return req;
}
