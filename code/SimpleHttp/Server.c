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
	//1.���������׽���
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1) {
		perror("socket");
		return -1;
	}
	//2.�����˿ڸ���
	int opt = 1;
	//ȷ�������������ܹ���������������������صȴ� TIME_WAIT ״̬�������ͷš�
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	//3.��IP�˿�
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;		//�󶨱��������IP��ַ
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1) {
		perror("bind");
		return -1;
	}
	//4.��������
	ret = listen(lfd, 128);
	if (ret == -1) {
		perror("listen");
		return -1;
	}
	//5.���ؼ����׽���
	return lfd;
}

int epollRun(int lfd)
{
	//1.����epollʵ��
	int epfd = epoll_create(1);
	if (epfd == -1) {
		perror("epoll_create");
		return -1;
	}
	//2.epoll�������ӽڵ�
		//���������¼�
	struct epoll_event ev;
	ev.data.fd = lfd;
	ev.events = EPOLLIN | EPOLLET;	//���¼�
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
	if (ret == -1) {
		perror("epoll_ctl");
		return -1;
	}
	//3.ѭ�����
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
				//��������
				//acceptClient(lfd, epfd);
				pthread_create(&info->tid, NULL, acceptClient, info);
			}
			else {
				//ͨ��
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
	//1.��������
	int cfd = accept(info->fd, NULL, NULL);
	if (cfd == -1) {
		perror("accept");
		return NULL;
	}
	//epoll���ط�����ģʽ
	//2.���÷�����
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(cfd, F_SETFL, flag);

	//3.cfd���ӵ�epoll��
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
	//����ģʽ����Ҫһ���Զ�ȡ��������
	while ((len = recv(info->fd, tmp, sizeof tmp, 0)) > 0) {
		//printf("len=%d\n", len);
		//ֻ��Ҫ��ȡ�������м��ɣ���������ݶ�ʧҲ����
		if (total + len < sizeof buf) {
			memcpy(buf + total, tmp, len);
		}
		total += len;
	}
	//recv�Ƿ���������˶�ȡʧ�ܻ�������ݾ��᷵��-1
	//�ж������Ƿ񱻳ɹ���ȡ
	if (len == -1 && errno == EAGAIN) {
		/*
		1.������
		2.����ͷ
		3.��
		4.��
		*/
		//����������	������get����
		char* pt = strstr(buf, "\r\n");
		int reqLen = pt - buf;
		buf[reqLen] = '\0';
		parseRequestLine(buf, info->fd);

	}
	else if (len == 0) {
		//�ͻ��˶Ͽ�������
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
	//����������
	char method[12];
	char path[1024];
	sscanf(line, "%[^ ] %[^ ]", method, path);
	printf("method:%s\tpath=%s\n", method, path);
	if (strcasecmp(method, "get") != 0) {
		//��������
		return -1;
	}
	decodeMsg(path, path);
	printf("backpath:%s\n", path);
	//�����ͻ�������ľ�̬��Դ(Ŀ¼�����ļ�)
	char* file = NULL;
	if (strcmp(path, "/") == 0) {
		file = "./";
	}
	else {
		file = path + 1;
	}
	//��ȡ�ļ�����
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1) {
		//�ļ�������	����404
		sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);	//ָ������-1������Զ���ȡ����
		sendFile("404.html", cfd);
		return -1;
	}
	//�ж��ļ�����
	if (S_ISDIR(st.st_mode)) {
		//��Ŀ¼�е����ݷ��͸��ͻ���
		sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		sendDir(file, cfd);
	}
	else {
		//���ļ��е����ݷ��͸��ͻ���
		sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		sendFile(file, cfd);
	}

	return 0;
}

int sendFile(const char* fileName, int cfd)
{
	//1.���ļ�
	int fp = open(fileName, O_RDONLY);
	assert(fp);
#if 0
	while (0)
	{
		//ÿ�δ��ļ��ж�ȡ1k����
		char buf[1024];
		int len = read(fp, buf, sizeof buf);
		if (len > 0) {
			send(cfd, buf, len, 0);
			usleep(10);		//���Ʒ��Ͷ��ٶ�
		}
		else if (len == 0) {
			//���ݶ�ȡ���
			break;
		}
		else
		{
			perror("read");
		}

	}
#else
	off_t offset = 0;		//ʹ��int����ʧ��
	int size = lseek(fp, 0, SEEK_END);
	lseek(fp, 0, SEEK_SET);
	while (offset < size) {
		int ret=sendfile(cfd, fp, &offset, size);		//�ų��㿽��
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
	//״̬��
	sprintf(buf, "http/1.1 %d %s\r\n", status, descr);
	//��Ӧͷ
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
		//ȡ���ļ���
		char* name = namelist[i]->d_name;
		struct stat st;
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName, name);
		stat(subPath, &st);
		if (S_ISDIR(st.st_mode)) {
			//Ŀ¼������ʽ
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",			//Ŀ¼��Ҫ��/
				name, name, st.st_size);
		}
		else
		{
			//�ļ�������ʽ
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
	// ����������ҡ�.���ַ�, �粻���ڷ���NULL
	const char* dot = strrchr(name, '.');
	if (dot == NULL)
		return "text/plain; charset=utf-8";	// ���ı�
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

// ���ַ�ת��Ϊ������
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

// ����
// to �洢����֮�������, ��������, from�����������, �������
void decodeMsg(char* to, char* from)
{
	for (; *from != '\0'; ++to, ++from)
	{
		// isxdigit -> �ж��ַ��ǲ���16���Ƹ�ʽ, ȡֵ�� 0-f
		// Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// ��16���Ƶ��� -> ʮ���� �������ֵ��ֵ�����ַ� int -> char
			// B2 == 178
			// ��3���ַ�, �����һ���ַ�, ����ַ�����ԭʼ����
			*to = hexToDec(from[1]) * 16 + hexToDec(from[2]);

			// ���� from[1] �� from[2] ����ڵ�ǰѭ�����Ѿ���������
			from += 2;
		}
		else
		{
			// �ַ�����, ��ֵ
			*to = *from;
		}

	}
	*to = '\0';
}

