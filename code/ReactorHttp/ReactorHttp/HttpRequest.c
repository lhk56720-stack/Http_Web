#include "HttpRequest.h"
#include<stdio.h>
#include<strings.h>
#include<assert.h>
#include<sys/stat.h>
#include<dirent.h>
#include<fcntl.h>
#include<unistd.h>
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
//拆分请求行
char* splitRequestLine(const char* start,const char* end,const char* sub,char** ptr) {
	char* space = end;
	if (sub != NULL) {
		space = memmem(start, end - start, sub, strlen(sub));
		assert(space != NULL);
	}
	int length = space - start;
	char* tmp = (char*)malloc(length + 1);	//给\0留空间
	strncpy(tmp, start, length);
	tmp[length] = '\0';
	*ptr = tmp;
	return space + 1;
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
		start = splitRequestLine(start, end, " ", &req->method);
		start = splitRequestLine(start, end, " ", &req->url);
		splitRequestLine(start, end, NULL, &req->version);
#if 0
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
#endif

		//为解析请求头做准备
		readBuf->readPos += lineSize+2;
		//修改当前状态
		req->curStatus++;
		return true;
	}
	return false;
}

bool parseHttpRequestHeader(struct HttpRequest* req, struct Buffer* readBuf)
{
	char* end = bufferFindLine(readBuf);
	if (end != NULL) {
		char* start = readBuf->data + readBuf->readPos;
		int lineSize = end - start;
		//基于冒号搜索
		char* mid = memmem(start, lineSize, ": ", 2);
		if (mid != NULL) {
			char* key = malloc(mid - start + 1);
			strncpy(key, start, mid - start);
			key[mid - start] = '\0';

			char* value = malloc(end - mid - 2);
			strncpy(value, mid + 2, end - mid - 2);
			key[end - mid - 2] = '\0';

			httpRequestAddHeader(req, key, value);
			//为下一段解析做准备
			readBuf->readPos += lineSize + 2;
		}
		else
		{
			//请求头解析完毕，跳过空行
			readBuf->readPos += 2;
			//修改解析状态
			req->curStatus = ParseReqDone; 
		}
		return true;
	}
	return false;
}

bool parseHttpRequest(struct HttpRequest* req, struct Buffer* readBuf, struct HttpResponse* response, struct Buffer* sendBuf, int socket)
{
	bool flag = true;
	while (req->curStatus!=ParseReqDone)
	{
		switch (req->curStatus) {
		case ParseReqLine:
			flag=parseHttpRequestLine(req, readBuf);
			break;
		case ParseReqHead:
			flag= parseHttpRequestHeader(req, readBuf);
		case ParseReqBody:
			break;
		case ParseReqDone:
			break;
		default:
			break;	
		}
		if (!flag)
			return flag;
		//判断是否解析完毕，完毕则做出数据回复
		if (req->curStatus == ParseReqDone) {
			//根据解析出的数据对客户端的请求做出处理
			processHttpRequest(req,response);
			//组织响应数据发送给客户端
			httpResponsePrepareMsg(response, sendBuf, socket);
		}
	}
	//状态还原
	req->curStatus = ParseReqLine;
	return flag;
}

//处理基于get的http请求
int processHttpRequest(struct HttpRequest* req, struct HttpResponse* response)
{
	if (strcasecmp(req->method, "get") != 0) {
		//不做处理
		return -1;
	}
	decodeMsg(req->url, req->url);
	//处理客户端请求的静态资源(目录或者文件)
	char* file = NULL;
	if (strcmp(req->url, "/") == 0) {
		file = "./";
	}
	else {
		file = req->url + 1;
	}
	//获取文件属性
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1) {
		//文件不存在	返回404
		//sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);	//指定参数-1浏览器自动读取长度
		//sendFile("404.html", cfd);
		strcpy(response->fileName, "404.html");
		response->statusCode = NotFound;
		strcpy(response->StatusMsg, "Not Found");
		//响应头
		httpResponseAddHeader(response, "Content-type", getFileType(".html"));
		response->sendDataFunc = sendFile;
		return 0;
	}
	strcpy(response->fileName, file);
	response->statusCode = OK;
	strcpy(response->StatusMsg, "OK");
	//判断文件类型
	if (S_ISDIR(st.st_mode)) {
		//把目录中的内容发送给客户端
		/*sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		sendDir(file, cfd);*/
		//响应头
		httpResponseAddHeader(response, "Content-type", getFileType(".html"));
		response->sendDataFunc = sendDir;
	}
	else {
		//把文件中的内容发送给客户端
		/*sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		sendFile(file, cfd);*/
		//响应头
		char tmp[16] = { 0 };
		sprintf("%ld", st.st_size);
		httpResponseAddHeader(response, "Content-type", getFileType(file));
		httpResponseAddHeader(response, "Content-length", tmp);
		response->sendDataFunc = sendFile;
	}

	return 0;
}


void sendFile(const char* fileName, struct Buffer* sendBuf, int cfd)
{
	//1.打开文件
	int fp = open(fileName, O_RDONLY);
	assert(fp);
#if 1
	while (0)
	{
		//每次从文件中读取1k数据
		char buf[1024];
		int len = read(fp, buf, sizeof buf);
		if (len > 0) {
			//send(cfd, buf, len, 0);
			bufferAppendData(sendBuf, buf, len);
#ifndef MSG_SEND_AUTO
			bufferSendData(sendBuf, cfd);
#endif
			usleep(10);		//控制发送端速度
		}
		else if (len == 0) {
			//数据读取完毕
			break;
		}
		else
		{
			close(fp);
			perror("read");
		}

	}
#else
	off_t offset = 0;		//使用int加载失败
	int size = lseek(fp, 0, SEEK_END);
	lseek(fp, 0, SEEK_SET);
	while (offset < size) {
		int ret = sendfile(cfd, fp, &offset, size);		//号称零拷贝
		if (ret == -1) {
			perror("sendfile");
		}
	}

#endif
	close(fp);
	return 0;
}

void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd)
{
	char buf[1024 * 4];
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
	struct dirent** namelist;
	int num = scandir(dirName, &namelist, NULL, alphasort);
	for (int i = 0; i < num; ++i) {
		//取出文件名
		char* name = namelist[i]->d_name;
		struct stat st;
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName, name);
		stat(subPath, &st);
		if (S_ISDIR(st.st_mode)) {
			//目录处理方式
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",			//目录需要加/
				name, name, st.st_size);
		}
		else
		{
			//文件处理方式
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);

		}
		//send(cfd, buf, strlen(buf), 0);
		bufferAppendData(sendBuf, buf, strlen(buf));
#ifndef MSG_SEND_AUTO
		bufferSendData(sendBuf, cfd);
#endif
		memset(buf, 0, sizeof buf);
		free(namelist[i]);
	}
	sprintf(buf, "</table></body></html>");
	//send(cfd, buf, strlen(buf), 0);
	bufferAppendData(sendBuf, buf, strlen(buf));
#ifndef MSG_SEND_AUTO
	bufferSendData(sendBuf, cfd);
#endif
	free(namelist);
	return 0;
}

// 将字符转换为整形数
int hexToDec(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return 0;
}

// 解码
// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
void decodeMsg(char* to, char* from)
{
	for (; *from != '\0'; ++to, ++from)
	{
		// isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
		// Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
			// B2 == 178
			// 将3个字符, 变成了一个字符, 这个字符就是原始数据
			*to = hexToDec(from[1]) * 16 + hexToDec(from[2]);

			// 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
			from += 2;
		}
		else
		{
			// 字符拷贝, 赋值
			*to = *from;
		}

	}
	*to = '\0';
}

const char* getFileType(const char* name)
{
	// a.jpg a.mp4 a.html
	// 自右向左查找‘.’字符, 如不存在返回NULL
	const char* dot = strrchr(name, '.');
	if (dot == NULL)
		return "text/plain; charset=utf-8";	// 纯文本
	if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
		return "text/html; charset=utf-8";
	if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
		return "image/jpeg";
	if (strcmp(dot, ".gif") == 0)
		return "image/gif";
	if (strcmp(dot, ".png") == 0)
		return "image/png";
	if (strcmp(dot, ".css") == 0)
		return "text/css";
	if (strcmp(dot, ".au") == 0)
		return "audio/basic";
	if (strcmp(dot, ".wav") == 0)
		return "audio/wav";
	if (strcmp(dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
		return "video/quicktime";
	if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
		return "model/vrml";
	if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
		return "audio/midi";
	if (strcmp(dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcmp(dot, ".ogg") == 0)
		return "application/ogg";
	if (strcmp(dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";

	return "text/plain; charset=utf-8";
}


