#pragma once
#include "Channel.h"
#include <stdlib.h>
struct ChannelMap
{
	int size; // 记录指针指向的数组的元素的总个数
	struct Channel **list;
};
// 初始化
struct ChannelMap *channelMapInit(int size);
// 清空
void ChannelMapClear(struct ChannelMap *map);
// 重新分配内存空间
bool makeMapRoom(struct ChannelMap *map, int newSize, int unitSize);