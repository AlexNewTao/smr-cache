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

    //偏移量赋值
	ssd_buf_tag.offset = offset;

   	if (DEBUG)
        printf("[INFO] write():-------offset=%lu\n", offset);

    //根据ssd_buf_tag，分配空间
    ssd_buf_hdr = SSDBufferAlloc(ssd_buf_tag, &found);

	returnCode = pwrite(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
	if(returnCode < 0) {            
		printf("[ERROR] write():-------write to ssd: fd=%d, errorcode=%d, offset=%lu\n", ssd_fd, returnCode, offset);
		exit(-1);
	}
        ssd_buf_hdr->ssd_buf_flag |= SSD_BUF_VALID | SSD_BUF_DIRTY;
}


//根据ssd_buf_tag，分配空间

static SSDBufferDesc * SSDBufferAlloc(SSDBufferTag ssd_buf_tag, bool *found)
{
	SSDBufferDesc *ssd_buf_hdr;

	//首先根据ssd_buf_tag得到ssd hash值
    unsigned long ssd_buf_hash = ssdbuftableHashcode(&ssd_buf_tag);

    //根据hash值查找id号
    long ssd_buf_id = ssdbuftableLookup(&ssd_buf_tag, ssd_buf_hash);

    //如果找到
    if (ssd_buf_id >= 0) {
        ssd_buf_hdr = &ssd_buffer_descriptors[ssd_buf_id];
        //更改标志位
       	*found = 1;

        hitInSSDBuffer(ssd_buf_hdr, EvictStrategy);

        return ssd_buf_hdr;
    }

    //得到ssd的buffer；有clock策略和lru策略
	ssd_buf_hdr = getSSDStrategyBuffer(EvictStrategy);

    unsigned char old_flag = ssd_buf_hdr->ssd_buf_flag;
    
    SSDBufferTag old_tag = ssd_buf_hdr->ssd_buf_tag;
        

    if (DEBUG)
        printf("[INFO] SSDBufferAlloc(): old_flag&SSD_BUF_DIRTY=%d\n", old_flag & SSD_BUF_DIRTY);
    
    //标志位表示为脏数据
    if (old_flag & SSD_BUF_DIRTY != 0) {
        flushSSDBuffer(ssd_buf_hdr);
    }
    //标志位为可利用
    if (old_flag & SSD_BUF_VALID != 0) {
    	//得到hashcode
        unsigned long old_hash = ssdbuftableHashcode(&old_tag);
        //删除
        ssdbuftableDelete(&old_tag, old_hash);
    }

    //插入到ssd buffer，并更改标志位
    ssdbuftableInsert(&ssd_buf_tag, ssd_buf_hash, ssd_buf_hdr->ssd_buf_id);

    ssd_buf_hdr->ssd_buf_flag &= ~(SSD_BUF_VALID | SSD_BUF_DIRTY);
    ssd_buf_hdr->ssd_buf_tag = ssd_buf_tag;
    *found = 0;

    return ssd_buf_hdr;
}


//命中方式函数；根据不同的策略，选择不同的调用方式
static void * hitInSSDBuffer(SSDBufferDesc * ssd_buf_hdr, SSDEvictionStrategy strategy)
{
	//如果采用的是clock的方式
    if (strategy == CLOCK)
        hitInCLOCKBuffer(ssd_buf_hdr);
    else if (strategy == LRU)//如果采用的是LRU的调用方式
        hitInLRUBuffer(ssd_buf_hdr);
}

//得到ssd的buffer
static SSDBufferDesc * getSSDStrategyBuffer(SSDEvictionStrategy strategy)
{
	//有两种策略，根据策略不同得到不同的buffer
	if (strategy == CLOCK)
		return getCLOCKBuffer();
    else if (strategy == LRU)
        return getLRUBuffer();
}


//把数据flush到smr
static volatile void* flushSSDBuffer(SSDBufferDesc *ssd_buf_hdr)
{
	char	ssd_buffer[SSD_BUFFER_SIZE];

	int 	returnCode;

	//把buffer的数据读出
	returnCode = pread(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
	//错误判断
	if(returnCode < 0) {            
		printf("[ERROR] flushSSDBuffer():-------read from ssd: fd=%d, errorcode=%d, offset=%lu\n", ssd_fd, returnCode, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
		exit(-1);
	}
	//把数据写到smr中
	returnCode = smrwrite(smr_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_tag.offset);
	//turnCode = pwrite(smr_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_tag.offset);
	
	//判断写入是否成功
	if(returnCode < 0) {            
		printf("[ERROR] flushSSDBuffer():-------write to smr: fd=%d, errorcode=%d, offset=%lu\n", ssd_fd, returnCode, ssd_buf_hdr->ssd_buf_tag.offset);
		exit(-1);
	}

    return NULL;
}


void read_block(off_t offset, char* ssd_buffer)
{
	void	*ssd_buf_block;
	bool	found = 0;
	int 	returnCode;

	static SSDBufferTag ssd_buf_tag;
    
    static SSDBufferDesc *ssd_buf_hdr;

	ssd_buf_tag.offset = offset;
        
    if (DEBUG)
        printf("[INFO] read():-------offset=%lu\n", offset);
    
    //分配空间
    ssd_buf_hdr = SSDBufferAlloc(ssd_buf_tag, &found);
    
    if (found) {
		returnCode = pread(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
		if(returnCode < 0) {            
			printf("[ERROR] read():-------read from smr: fd=%d, errorcode=%d, offset=%lu\n", ssd_fd, returnCode, offset);
			exit(-1);
		}    
    }
    else {
		returnCode = smrread(smr_fd, ssd_buffer, SSD_BUFFER_SIZE, offset);
		//returnCode = pread(smr_fd, ssd_buffer, SSD_BUFFER_SIZE, offset);
		if(returnCode < 0) {            
			printf("[ERROR] read():-------read from smr: fd=%d, errorcode=%d, offset=%lu\n", ssd_fd, returnCode, offset);
			exit(-1);
		} 
		//取数到ssd中   
		returnCode = pwrite(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
		if(returnCode < 0) {            
			printf("[ERROR] read():-------write to ssd: fd=%d, errorcode=%d, offset=%lu\n", ssd_fd, returnCode, offset);
			exit(-1);
		}    

        }
        //更改标志位
        ssd_buf_hdr->ssd_buf_flag &= ~SSD_BUF_VALID;
        ssd_buf_hdr->ssd_buf_flag |= SSD_BUF_VALID;
}
