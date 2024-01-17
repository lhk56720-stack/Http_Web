#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "TcpServer.h"
/*
 * argv[0]:可执行程序的名字
 * argv[1]:传递进来的第一个参数，服务器绑定的端口
 * argv[2]:传递进来的第一个参数，服务器访问的资源目录
 */
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("./ReactorHttp.out port path\n");
        return -1;
    }
    // atoi() 将数字字符的字符串转换为整形
    unsigned short port = (unsigned short)atoi(argv[1]);
    // 切换服务器的工作目录至用户指定路径
    chdir(argv[2]);
    // 启动服务器
    struct TcpServer *server = tcpServerInit(port, 4);
    tcpServerRun(server);

    return 0;
}