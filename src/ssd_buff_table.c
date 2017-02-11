

#include <stdio.h>
#include <stdlib.h>

#include "ssd-cache.h"



void initSSDBufTable(size_t size)
{
	ssd_buffer_hashtable = (SSDBufferHashBucket *)malloc(sizeof(SSDBufferHashBucket)*size);
	size_t i;
	SSDBufferHashBucket *ssd_buf_hash = ssd_buffer_hashtable;
	for (i = 0; i < size; ssd_buf_hash++, i++){
		ssd_buf_hash->ssd_buf_id = -1;
		ssd_buf_hash->hash_key.offset = -1;
		ssd_buf_hash->next_item = NULL;
	}
}