#define _GNU_SOURCE
#include "Buffer.h"
#include<string.h>
#include<sys/uio.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<sys/socket.h>
struct Buffer* bufferInit(int size)
{
	struct Buffer* buffer = (struct Buffer*)malloc(sizeof(struct Buffer));
	buffer->data = (char*)malloc(size);
	memset(buffer->data, 0, size);
	buffer->capacity = size;
	buffer->readPos = buffer->writePos = 0;
	return buffer;
}

void bufferDestroy(struct Buffer* buf)
{
	if (buf != NULL) {
		if (buf->data != NULL) {
			free(buf->data);
			buf->data = NULL;
		}
		free(buf);
		buf = NULL;
	}
}

void bufferExtendRoom(struct Buffer* buf,int newSize)
{
	//1.内存足够，无需扩容
	if (getBufferWriteSize(buf) >= newSize)
		return;
	//2.内存合并后够用，无需扩容
	else if (buf->readPos + getBufferWriteSize(buf) >= newSize) {
		//得到未读的内存块
		int readEnable = getBufferReadSize(buf);
		//移动内存块
		memcpy(buf->data, buf->data + buf->readPos, readEnable);
		//更新
		buf->readPos = 0;
		buf->writePos = readEnable;
	}
	//3.内存不够，需要扩容
	else {
		void* tmp = realloc(buf->data, buf->capacity + newSize);
		if (tmp==NULL)
			return;
		memset(tmp + buf->capacity, 0, newSize);
		//更新数据
		buf->data = tmp;
		buf->capacity += newSize;
	}
}

int getBufferReadSize(struct Buffer* buf)
{
	return buf->writePos - buf->readPos;
}

int getBufferWriteSize(struct Buffer* buf)
{
	return buf->capacity - buf->writePos;
}

int bufferAppendData(struct Buffer* buf, const char* data, int size)
{
	if (buf == NULL || data == NULL || data <= 0)
		return -1;
	//扩容
	bufferExtendRoom(buf, size);
	memcpy(buf->data + buf->writePos, data, size);
	buf->writePos += size;
	return 0;
}

int bufferAppendString(struct Buffer* buffer, const char* data)
{
	int size = strlen(data);
	int ret = bufferAppendData(buffer, data, size);
	return ret;
}

int bufferSocketRead(struct Buffer* buf, int fd)
{
	struct iovec vec[2];
	int writeEnable = getBufferWriteSize(buf);
	vec[0].iov_base = buf->data + buf->writePos;
	vec[0].iov_len = writeEnable;
	char* tmpBuf = (char*)malloc(40960);
	vec[1].iov_base = tmpBuf;
	vec[1].iov_len = 40960;
	int ret = readv(fd, vec, 2);
	if (ret == -1) {
		perror("readv");
		return -1;
	}
	else if (ret <= writeEnable) {
		buf->writePos += ret;
	}
	else {
		buf->writePos = buf->capacity;
		bufferAppendData(buf, tmpBuf, ret - writeEnable);
	}
	free(tmpBuf);
	tmpBuf = NULL;

	return ret;
}

char* bufferFindLine(struct Buffer* buf)
{
	//strstr	大字符串中匹配子字符串(遇到\0结束）
	//memmem	大数据块中匹配子数据块(需要指定子数据块大小)
	char* ptr=memmem(buf->data + buf->readPos, getBufferReadSize(buf), "\r\n", 2);
	return ptr;
}

int bufferSendData(struct Buffer* buf, int socket)
{
	//判断有无数据
	int readEnable = getBufferReadSize(buf);
	if (readEnable > 0) {
		int count = send(socket, buf->data + buf->readPos, readEnable, MSG_NOSIGNAL);
		if (count) {
			buf->readPos += count;
			usleep(1);
		}
		return count;
	}
	return 0;
}
