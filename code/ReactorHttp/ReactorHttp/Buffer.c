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
