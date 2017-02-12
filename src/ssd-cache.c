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


static void * initStrategySSDBuffer(SSDEvictionStrategy strategy)
{
    if (strategy == CLOCK)
        initSSDBufferForClock();
    else if (strategy == LRU)
        initSSDBufferForLRU();
}


/*
 * write--return the buf_id of buffer according to buf_tag
 */

//写block数据
void write_block(off_t offset, char* ssd_buffer)
{
    void *ssd_buf_block;
    bool found;
	int  returnCode;

	static SSDBufferTag ssd_buf_tag;
        static SSDBufferDesc *ssd_buf_hdr;

	ssd_buf_tag.offset = offset;
        if (DEBUG)
                printf("[INFO] write():-------offset=%lu\n", offset);
        ssd_buf_hdr = SSDBufferAlloc(ssd_buf_tag, &found);
	returnCode = pwrite(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
	if(returnCode < 0) {            
		printf("[ERROR] write():-------write to ssd: fd=%d, errorcode=%d, offset=%lu\n", ssd_fd, returnCode, offset);
		exit(-1);
	}
        ssd_buf_hdr->ssd_buf_flag |= SSD_BUF_VALID | SSD_BUF_DIRTY;
}




