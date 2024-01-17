#include <stdio.h>
#include "Server.h"
#include <unistd.h>
#include <stdlib.h>
/*
 * argv[0]:可执行程序的名字
 * argv[1]:传递进来的第一个参数，服务器绑定的端口
 * argv[2]:传递进来的第一个参数，服务器访问的资源目录
 */
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("./SimpleHttp port path\n");
        return -1;
    }
    // atoi() 将数字字符的字符串转换为整形
    unsigned short port = (unsigned short)atoi(argv[1]);
    // 切换服务器的工作目录至用户指定路径
    chdir(argv[2]);
    // 初始化用于监听的套接字
    int lfd = initListenFd(port);
    // 启动服务器 epoll
    epollRun(lfd);

    return 0;
}