#include "TcpServer.h"
#include<arpa/inet.h>
struct TcpServer* tcpServerInit(unsigned short port, int threadNum)
{
	struct TcpServer* tcpServer = (struct TcpServer*)malloc(sizeof(struct TcpServer));
	tcpServer->mainLoop = eventLoopInit();
	tcpServer->threadNum = threadNum;
	tcpServer->threadPool = threadPoolInit(tcpServer->mainLoop, threadNum);
	tcpServer->listener = tcpServerInit(port,threadNum);
	return tcpServer;
}

struct Listener* listenerInit(unsigned short port)
{
	struct Listener* listener = (struct Listener*)malloc(sizeof(struct Listener));
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
	//5.返回
	listener->lfd = lfd;
	listener->port = port;
	return listener;
}
