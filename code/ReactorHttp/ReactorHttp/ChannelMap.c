#include "ChannelMap.h"
#include<string.h>
struct ChannelMap* channelMapInit(int size)
{
	struct ChannelMap* map = (struct ChannelMap*)malloc(sizeof(struct ChannelMap));
	map->size = size;
	map->list = (struct Channel**)malloc(size * sizeof(struct Channel*));
	return map;
}

void ChannelMapClear(struct ChannelMap* map)
{
	if (map) {
		for (int i = 0; i < map->size; ++i) {
			if (map->list[i]) {
				free(map->list[i]);
			}
		}
		free(map->list);
		map->list = NULL;
	}
	map->size = 0;
}

bool makeMapRoom(struct ChannelMap* map, int newSize, int unitSize)
{
	int curSize = map->size;
	//容量每次扩大一倍直至满足newSize
	while (curSize > newSize) {
		curSize *= 2;
	}
	//realloc重新分配内存后会返回新内存块的首地址
	struct Channel** tmplist = realloc(map->list, curSize * unitSize);
	if (tmplist) {
		map->list = tmplist;
		//初始化新扩展出的内存
		memset(&map->list[map->size], 0, (curSize - map->size) * unitSize);
		map->size = newSize;
		return true;
	}
	return false;
}

