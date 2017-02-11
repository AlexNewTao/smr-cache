
#define DEBUG 0
/* ---------------------------clock---------------------------- */

//ssd buffer 描述符
typedef struct
{
	//id号
	long 		ssd_buf_id;				// ssd buffer location in shared buffer
	//使用数目
	unsigned long	usage_count;
} SSDBufferDescForClock;


typedef struct
{
	long		next_victimssd;		// For CLOCK
} SSDBufferStrategyControlForClock;


SSDBufferDescForClock	*ssd_buffer_descriptors_for_clock;
SSDBufferStrategyControlForClock *ssd_buffer_strategy_control_for_clock;

