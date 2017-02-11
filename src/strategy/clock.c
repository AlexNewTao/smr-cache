
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
