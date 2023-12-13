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