
#include <stdio.h>
#include <stdlib.h>
#include "../ssd-cache.h"
#include "../smr-simulator/smr-simulator.h"
#include "clock.h"

/*
 * init strategy_control for clock
 	增加时钟访问策略
 */
void initSSDBufferForClock()
{
	//分配空间
	ssd_buffer_strategy_control_for_clock = (SSDBufferStrategyControlForClock *) malloc(sizeof(SSDBufferStrategyControlForClock));
	//变量初始化为0
	ssd_buffer_strategy_control_for_clock->next_victimssd = 0;

	//为ssd描述符分配空间
	ssd_buffer_descriptors_for_clock = (SSDBufferDescForClock *) malloc(sizeof(SSDBufferDescForClock)*NSSDBuffers);
	SSDBufferDescForClock *ssd_buf_hdr_for_clock;
	long i;
	ssd_buf_hdr_for_clock = ssd_buffer_descriptors_for_clock;
	//遍历初始化，分配id号
	for (i = 0; i < NSSDBuffers; ssd_buf_hdr_for_clock++, i++) {
		ssd_buf_hdr_for_clock->ssd_buf_id = i;
		ssd_buf_hdr_for_clock->usage_count = 0;
	}
}

//采用clock的方式调用
void *hitInCLOCKBuffer(SSDBufferDesc *ssd_buf_hdr)
{
    SSDBufferDescForClock *ssd_buf_hdr_for_clock;
    ssd_buf_hdr_for_clock = &ssd_buffer_descriptors_for_clock[ssd_buf_hdr->ssd_buf_id];
    ssd_buf_hdr_for_clock->usage_count++;

    return NULL;
}

//得到clock策略的buffer
SSDBufferDesc *getCLOCKBuffer()
{
	SSDBufferDescForClock *ssd_buf_hdr_for_clock;

	SSDBufferDesc *ssd_buf_hdr;

	//如果free ssd的链表头存在
	if (ssd_buffer_strategy_control->first_freessd >=0 ) {
		//把地址传给ssd_buf_hdr
		ssd_buf_hdr = &ssd_buffer_descriptors[ssd_buffer_strategy_control->first_freessd];

		ssd_buffer_strategy_control->first_freessd = ssd_buf_hdr->next_freessd;
		ssd_buf_hdr->next_freessd = -1;
        ssd_buffer_strategy_control->n_usedssd ++;
		return ssd_buf_hdr;
	}

	for (;;) {
		ssd_buf_hdr_for_clock = &ssd_buffer_descriptors_for_clock[ssd_buffer_strategy_control_for_clock->next_victimssd];

		ssd_buf_hdr = &ssd_buffer_descriptors[ssd_buffer_strategy_control_for_clock->next_victimssd];

		//按照clock方式后移
		ssd_buffer_strategy_control_for_clock->next_victimssd++;

		//限制条件判断
		if (ssd_buffer_strategy_control_for_clock->next_victimssd >= NSSDBuffers) {
			ssd_buffer_strategy_control_for_clock->next_victimssd = 0;
		}
		if (ssd_buf_hdr_for_clock->usage_count > 0) {
			ssd_buf_hdr_for_clock->usage_count--;
		}
		else
			return ssd_buf_hdr;
	}
	
	return NULL;
}


