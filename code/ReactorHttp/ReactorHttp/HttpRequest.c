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
