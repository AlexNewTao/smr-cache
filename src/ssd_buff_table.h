#ifndef SSDBUFTABLE_H
#define SSDBUFTABLE_H

typedef struct SSDBufferHashBucket
{
	SSDBufferTag 			hash_key;
	long    				ssd_buf_id;
	struct SSDBufferHashBucket 	*next_item;
} SSDBufferHashBucket;





extern void initSSDBufTable(size_t size);


#endif


