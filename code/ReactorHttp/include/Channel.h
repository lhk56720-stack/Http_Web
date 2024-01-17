#pragma once
#include <stdbool.h>
// 定义函数指针
typedef int (*handleFunc)(void *arg);
// 定义文件描述符的读写事件
enum FDEvent
{
	ReadEvent = 0x02,
	WriteEvent = 0x04
};
struct Channel
{
	// 文件描述符
	int fd;
	// 事件
	int events;
	// 回调函数
	handleFunc readCallback;  // 读事件
	handleFunc writeCallback; // 写事件
	handleFunc destroyCallback;
	// 回调函数的参数
	void *arg;
};
// 初始化Channel
struct Channel *channelInit(int fd, int events, handleFunc readFunc, handleFunc writeFun, handleFunc destroyFunc, void *arg);
// 判断是否需要检测文件描述符的写事件
bool isWriteEventCheck(struct Channel *channel);
// 修改fd的写事件（检测|不检测）
void writeEventEnable(struct Channel *channel, bool flag);