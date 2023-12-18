#include "TcpServer.h"
#include<arpa/inet.h>
#include"TcpConnection.h"
#include<stdio.h>
struct TcpServer* tcpServerInit(unsigned short port, int threadNum)
{
	struct TcpServer* tcpServer = (struct TcpServer*)malloc(sizeof(struct TcpServer));
	tcpServer->mainLoop = eventLoopInit();
	tcpServer->threadNum = threadNum;
	tcpServer->threadPool = threadPoolInit(tcpServer->mainLoop, threadNum);
	tcpServer->listener = listenerInit(port);
	return tcpServer;
}

struct Listener* listenerInit(unsigned short port)
{
	struct Listener* listener = (struct Listener*)malloc(sizeof(struct Listener));
	//1.创建监听套接字
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1) {
		perror("socket");
		return NULL;
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
		return NULL;
	}
	//4.启动监听
	ret = listen(lfd, 128);
	if (ret == -1) {
		perror("listen");
		return NULL;
	}
	//5.返回
	listener->lfd = lfd;
	listener->port = port;
	return listener;
}
int acceptConnection(void* arg) {
	struct TcpServer* tcp = (struct TcpServer*)arg;
	//和客户端建立连接
	int cfd = accept(tcp->listener->lfd, NULL, NULL);
	//从线程池中取出一个子线程的反应堆实例去处理这个cfd
	struct EventLoop* evLoop = takeWorkerEventLoop(tcp->threadPool);
	//把cfd放到TcpConnection中处理
	tcpConnectionInit(cfd, evLoop);
	return 0;
}
void tcpServerRun(struct TcpServer* tcp)
{
	//启动线程池
	threadPoolRun(tcp->threadPool);
	//实例化channel
	struct Channel* channel = channelInit(tcp->listener->lfd, ReadEvent, acceptConnection, NULL, NULL, tcp);
	//添加检测的任务
	eventLoopAddTask(tcp->mainLoop, channel, ADD);
	//启动反应堆模型
	eventLoopRun(tcp->mainLoop);
	printf("server start...\n");
}
