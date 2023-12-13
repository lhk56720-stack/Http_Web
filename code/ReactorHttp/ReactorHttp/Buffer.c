#include "Buffer.h"
#include<string.h>
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
		void* tmp = relloc(buf->data, buf->capacity + newSize);
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
