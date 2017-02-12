

#include <stdio.h>
#include <stdlib.h>

#include "ssd-cache.h"



void initSSDBufTable(size_t size)
{
	//分配空间
	ssd_buffer_hashtable = (SSDBufferHashBucket *)malloc(sizeof(SSDBufferHashBucket)*size);
	size_t i;
	SSDBufferHashBucket *ssd_buf_hash = ssd_buffer_hashtable;
	//参数初始化
	for (i = 0; i < size; ssd_buf_hash++, i++){
		ssd_buf_hash->ssd_buf_id = -1;
		ssd_buf_hash->hash_key.offset = -1;
		ssd_buf_hash->next_item = NULL;
	}
}

unsigned long ssdbuftableHashcode(SSDBufferTag *ssd_buf_tag)
{
	unsigned long ssd_buf_hash = (ssd_buf_tag->offset / SSD_BUFFER_SIZE) % NSSDBufTables;

	return ssd_buf_hash;
}


//在buffer中删除
long ssdbuftableDelete(SSDBufferTag *ssd_buf_tag, unsigned long hash_code)
{
	if (DEBUG)
		printf("[INFO] Delete buf_tag: %lu\n",ssd_buf_tag->offset);

	//根据hash code得到buffer中的bucket
	SSDBufferHashBucket *nowbucket = GetSSDBufHashBucket(hash_code);

	long del_id;
	SSDBufferHashBucket *delitem;
	nowbucket->next_item;

	while (nowbucket->next_item != NULL && nowbucket != NULL) {
		//如果找到
		if (isSamebuf(&nowbucket->next_item->hash_key, ssd_buf_tag)) {
			del_id = nowbucket->next_item->ssd_buf_id;
			break;
		}
		nowbucket = nowbucket->next_item;
	}
	//printf("not found2\n");
	//在bucket中判断最后一个
	if (isSamebuf(&nowbucket->hash_key, ssd_buf_tag)) {
		del_id = nowbucket->ssd_buf_id;
	}
	//printf("not found3\n");
	//如果需要删除的不为最后一个
	if (nowbucket->next_item != NULL) {
		delitem = nowbucket->next_item;
		nowbucket->next_item = nowbucket->next_item->next_item;
		free(delitem);
		return del_id;
	}
	else {
		//表示需要删除的为最后一个
		delitem = nowbucket->next_item;
		nowbucket->next_item = NULL;
		free(delitem);
		return del_id;
	}

	return -1;
}




