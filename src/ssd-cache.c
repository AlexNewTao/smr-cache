/*
    

    ssd cache function

*/


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include "ssd-cache.h"
#include "smr-simulator/smr-simulator.h"
#include "ssd_buf_table.h"
#include "strategy/clock.h"
#include "strategy/lru.h"


/*
 * init buffer hash table, strategy_control, buffer, work_mem

 */
void initSSDBuffer()
{
	//初始化SSD Buf Table，大小初始化为NSSDBufTables
	initSSDBufTable(NSSDBufTables);

	ssd_buffer_strategy_control = (SSDBufferStrategyControl *) malloc(sizeof(SSDBufferStrategyControl));
    ssd_buffer_strategy_control->n_usedssd = 0;
	ssd_buffer_strategy_control->first_freessd = 0;
	ssd_buffer_strategy_control->last_freessd = NSSDBuffers - 1;

	ssd_buffer_descriptors = (SSDBufferDesc *) malloc(sizeof(SSDBufferDesc)*NSSDBuffers);
	SSDBufferDesc *ssd_buf_hdr;
	long i;
	ssd_buf_hdr = ssd_buffer_descriptors;
	for (i = 0; i < NSSDBuffers; ssd_buf_hdr++, i++) {
		ssd_buf_hdr->ssd_buf_flag = 0;
		ssd_buf_hdr->ssd_buf_id = i;
		ssd_buf_hdr->next_freessd = i + 1;
	}
	ssd_buffer_descriptors[NSSDBuffers - 1].next_freessd = -1;

    initStrategySSDBuffer(EvictStrategy);
}








