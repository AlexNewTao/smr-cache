
#include <stdio.h>
#include <stdlib.h>
#include "../ssd-cache.h"
#include "../smr-simulator/smr-simulator.h"
#include "lru.h"


/*
 * init buffer hash table, strategy_control, buffer, work_mem
 */

//以lru策略初始化

void initSSDBufferForLRU()
{
	//分配空间，和clock方式类似
	ssd_buffer_strategy_control_for_lru = (SSDBufferStrategyControlForLRU *) malloc(sizeof(SSDBufferStrategyControlForLRU));
    
	//初始化参数，都为-1
    ssd_buffer_strategy_control_for_lru->first_lru = -1;
    ssd_buffer_strategy_control_for_lru->last_lru = -1;

    //未描述符分配空间
	ssd_buffer_descriptors_for_lru = (SSDBufferDescForLRU *) malloc(sizeof(SSDBufferDescForLRU)*NSSDBuffers);
	SSDBufferDescForLRU *ssd_buf_hdr_for_lru;
	long i;
	ssd_buf_hdr_for_lru = ssd_buffer_descriptors_for_lru;
	//分配id号
	for (i = 0; i < NSSDBuffers; ssd_buf_hdr_for_lru++, i++) {
		ssd_buf_hdr_for_lru->ssd_buf_id = i;
        ssd_buf_hdr_for_lru->next_lru = -1;
        ssd_buf_hdr_for_lru->last_lru = -1;
	}
}



void *hitInLRUBuffer(SSDBufferDesc *ssd_buf_hdr)
{
    moveToLRUHead(&ssd_buffer_descriptors_for_lru[ssd_buf_hdr->ssd_buf_id]);

    return NULL;
}











