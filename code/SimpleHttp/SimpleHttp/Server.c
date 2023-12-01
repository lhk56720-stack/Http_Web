#include "Server.h"
#include <arpa/inet.h>
#include<sys/epoll.h>
#include<stdio.h>
#include<fcntl.h>
#include<string.h>
#include<errno.h>
#include<strings.h>
#include<sys/stat.h>
#include<assert.h>
#include<sys/sendfile.h>
#include<dirent.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<ctype.h>
struct FdInfo
{
	int fd;
	int epfd;
	pthread_t tid;
};
int initListenFd(unsigned short port)
{
	//1.创建监听套接字
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1) {
		perror("socket");
		return -1;
	}
	//2.启动端口复用
	int opt = 1;
	//确保服务器程序能够更快地重新启动，而不必等待 TIME_WAIT 状态的连接释放。
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	//3.绑定IP端口
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;		//绑定本地任意的IP地址
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1) {
		perror("bind");
		return -1;
	}
	//4.启动监听
	ret = listen(lfd, 128);
	if (ret == -1) {
		perror("listen");
		return -1;
	}
	//5.返回监听套接字
	return lfd;
}

int epollRun(int lfd)
{
	//1.创建epoll实例
	int epfd = epoll_create(1);
	if (epfd == -1) {
		perror("epoll_create");
		return -1;
	}
	//2.epoll树上添加节点
		//创建上树事件
	struct epoll_event ev;
	ev.data.fd = lfd;
	ev.events = EPOLLIN | EPOLLET;	//读事件
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
	if (ret == -1) {
		perror("epoll_ctl");
		return -1;
	}
	//3.循环检测
	struct epoll_event evs[1024];
	int size = sizeof(evs) / sizeof(struct epoll_event);
	int i = 0;
	while (1) {
		printf("go in while...%d\n", i++);
		int num = epoll_wait(epfd, evs, size, -1);
		for (int i = 0; i < num; ++i) {
			struct FdInfo* info = (struct FdInfo*)malloc(sizeof(struct FdInfo));
			int fd = evs[i].data.fd;
			info->fd = fd;
			info->epfd = epfd;
			if (fd == lfd) {
				printf("fd==lfd......\n\n");
				//建立连接
				//acceptClient(lfd, epfd);
				pthread_create(&info->tid, NULL, acceptClient, info);
			}
			else {
				//通信
				//recvHttpRequest(fd, epfd);
				pthread_create(&info->tid, NULL, recvHttpRequest, info);
			}
		}
	}
	return 0;
}

void* acceptClient(void* arg)
{
	struct FdInfo* info = (struct FdInfo*)arg;
	printf("acceptClient thread id:%ld\n", info->tid);
	//1.建立连接
	int cfd = accept(info->fd, NULL, NULL);
	if (cfd == -1) {
		perror("accept");
		return NULL;
	}
	//epoll边沿非阻塞模式
	//2.设置非阻塞
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(cfd, F_SETFL, flag);

	//3.cfd添加到epoll中
	struct epoll_event ev;
	ev.data.fd = cfd;
	ev.events = EPOLLIN | EPOLLET;
	int ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev);
	if (ret == -1) {
		perror("epoll_ctl");
		return NULL;
	}
	free(info);
	return NULL;
}

void* recvHttpRequest(void* arg)
{
	struct FdInfo* info = (struct FdInfo*)arg;
	char buf[1024 * 4] = { 0 };
	char tmp[1024] = { 0 };
	int len = 0, total = 0;
	//边沿模式，需要一次性读取所有数据
	while ((len = recv(info->fd, tmp, sizeof tmp, 0)) > 0) {
		//printf("len=%d\n", len);
		//只需要读取到请求行即可，后面的内容丢失也可以
		if (total + len < sizeof buf) {
			memcpy(buf + total, tmp, len);
		}
		total += len;
	}
	//recv是非阻塞的因此读取失败或读完数据均会返回-1
	//判断数据是否被成功读取
	if (len == -1 && errno == EAGAIN) {
		/*
		1.请求行
		2.请求头
		3.空
		4.空
		*/
		//解析请求行	仅处理get请求
		char* pt = strstr(buf, "\r\n");
		int reqLen = pt - buf;
		buf[reqLen] = '\0';
		parseRequestLine(buf, info->fd);

	}
	else if (len == 0) {
		//客户端断开了连接
		epoll_ctl(info->epfd, EPOLL_CTL_DEL, info->fd, NULL);
		close(info->fd);
	}
	else {
		perror("recv");
		return NULL;
	}
	printf("recvHttpRequest thread id:%ld\n", info->tid);
	free(info);
	return NULL;
}

int parseRequestLine(const char* line, int cfd)
{
	//解析请求行
	char method[12];
	char path[1024];
	sscanf(line, "%[^ ] %[^ ]", method, path);
	printf("method:%s\tpath=%s\n", method, path);
	if (strcasecmp(method, "get") != 0) {
		//不做处理
		return -1;
	}
	decodeMsg(path, path);
	//处理客户端请求的静态资源(目录或者文件)
	char* file = NULL;
	if (strcmp(path, "/") == 0) {
		file = "./";
	}
	else {
		file = path + 1;
	}
	//获取文件属性
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1) {
		//文件不存在	返回404
		sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);	//指定参数-1浏览器自动读取长度
		sendFile("404.html", cfd);
		return -1;
	}
	//判断文件类型
	if (S_ISDIR(st.st_mode)) {
		//把目录中的内容发送给客户端
		sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		sendDir(file, cfd);
	}
	else {
		//把文件中的内容发送给客户端
		sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		sendFile(file, cfd);
	}

	return 0;
}

int sendFile(const char* fileName, int cfd)
{
	//1.打开文件
	int fp = open(fileName, O_RDONLY);
	assert(fp);
#if 0
	while (0)
	{
		//每次从文件中读取1k数据
		char buf[1024];
		int len = read(fp, buf, sizeof buf);
		if (len > 0) {
			send(cfd, buf, len, 0);
			usleep(10);		//控制发送端速度
		}
		else if (len == 0) {
			//数据读取完毕
			break;
		}
		else
		{
			perror("read");
		}

	}
#else
	off_t offset = 0;		//使用int加载失败
	int size = lseek(fp, 0, SEEK_END);
	lseek(fp, 0, SEEK_SET);
	while (offset < size) {
		int ret=sendfile(cfd, fp, &offset, size);		//号称零拷贝
		if (ret == -1) {
			perror("sendfile");
		}
	}
	
#endif
	close(fp);
	return 0;
}

int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length)
{
	char buf[1024 * 4] = { 0 };
	//状态行
	sprintf(buf, "http/1.1 %d %s\r\n", status, descr);
	//响应头
	sprintf(buf + strlen(buf), "content-type:%s\r\n", type);
	sprintf(buf + strlen(buf), "content-length:%d\r\n\r\n", length);
	send(cfd, buf, strlen(buf), 0);
	return 0;
}

int sendDir(const char* dirName, int cfd)
{
	char buf[1024 * 4];
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
	struct dirent** namelist;
	int num=scandir(dirName, &namelist, NULL, alphasort);
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
		send(cfd, buf, strlen(buf), 0);
		memset(buf, 0, sizeof buf);
		free(namelist[i]);
	}
	sprintf(buf+strlen(buf), "</table></body></html>");
	free(namelist);
	return 0;
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

