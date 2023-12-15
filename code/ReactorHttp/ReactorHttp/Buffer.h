#pragma once
struct Buffer {
	char* data;	//指向内存的指针
	int capacity;
	int readPos;
	int writePos;
};
//初始化Buffer
struct Buffer* bufferInit(int size);
//销毁buffer内存
void bufferDestroy(struct Buffer* buf);
//扩容
void bufferExtendRoom(struct Buffer* buf, int newSize);
//得到剩余的可读的内存容量
int getBufferReadSize(struct Buffer* buf);
//得到剩余的可写的内存容量
int getBufferWriteSize(struct Buffer* buf);
//1.直接写内存
int bufferAppendData(struct Buffer* buf, const char* data, int size);
//2.接收套接字数据
int bufferSocketRead(struct Buffer* buf, int fd);
//根据\r\n取出一行，找到其在数据块中的位置，返回位置信息
char* bufferFindLine(struct Buffer* buf);
//发送数据
int bufferSendData(struct Buffer* buf, int socket);