



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <memory.h>




#include "../ssd-cache.h"
#include "smr-simulator.h"
#include "inner_ssd_buf_table.h"


//声明函数
static SSDDesc *getStrategySSD();

static void* freeStrategySSD();

static volatile  void* flushSSD(SSDDesc *ssd_hdr);

static unsigned long GetSMRBandNumFromSSD(SSDDesc *ssd_hdr);

static off_t GetSMROffsetInBandFromSSD(SSDDesc *ssd_hdr);


/*
 * init inner ssd buffer hash table, strategy_control, buffer, work_mem
 */
void initSSD()
{
	// 申明线程id
    pthread_t freessd_tid;
    
    //初始化SSDtable
	initSSDTable(NSSDTables);

	ssd_strategy_control = (SSDStrategyControl *) malloc(sizeof(SSDStrategyControl));
	ssd_strategy_control->first_usedssd = 0;
	ssd_strategy_control->last_usedssd = -1;
	ssd_strategy_control->n_usedssd = 0;
	
	ssd_descriptors = (SSDDesc *) malloc(sizeof(SSDDesc)*NSSDs);
	SSDDesc *ssd_hdr;
	long i;
	ssd_hdr = ssd_descriptors;
	for (i = 0; i < NSSDs; ssd_hdr++, i++) {
		ssd_hdr->ssd_flag = 0;
		ssd_hdr->ssd_id = i;
//		ssd_hdr->usage_count = 0;
//		ssd_hdr->next_freessd = i + 1;
        }
//	ssd_descriptors[NSSDs - 1].next_freessd = -1;
	interval_time = 0;

	//为ssd分配空间，并且初始化为0
	ssd_blocks = (char *) malloc(SSD_SIZE*NSSDs);
	memset(ssd_blocks, 0, SSD_SIZE*NSSDs);

	//初始化释放ssd空间进程
    pthread_mutex_init(&free_ssd_mutex, NULL);
//    pthread_mutex_init(&inner_ssd_hdr_mutex, NULL);
//    pthread_mutex_init(&inner_ssd_table_mutex, NULL);
	int err;

    err = pthread_create(&freessd_tid, NULL, freeStrategySSD, NULL);
    if (err != 0) {
        printf("[ERROR] initSSD: fail to create thread: %s\n", strerror(err));
    }
}


//定义的内部的空间释放函数，在线程被创建的时候，定义的；

static void* freeStrategySSD()
{
	long i;

	while (1) {
		//进程挂起100微秒
		usleep(100);
		//全局变量间隔时间interval_time自增1
		interval_time++;

		//判断执行clean操作的时机
		/*
		interval_time > INTERVALTIMELIMIT 表示间隔时间大于规定的间隔限制时间
		ssd_strategy_control->n_usedssd >= NSSDCLEAN 表示ssd策略中已经使用的ssd数目大于规定的开始clean操作的数目
		ssd_strategy_control->n_usedssd >= NSSDLIMIT 表示ssd策略中已经使用的ssd数目大于限制的数目

		*/
		if ((interval_time > INTERVALTIMELIMIT && ssd_strategy_control->n_usedssd >= NSSDCLEAN)|| ssd_strategy_control->n_usedssd >= NSSDLIMIT) {
            if (DEBUG) {
			    printf("[INFO] freeStrategySSD():--------interval_time=%ld\n", interval_time);
                printf("[INFO] freeStrategySSD():--------ssd_strategy_control->n_usedssd=%lu ssd_strategy_control->first_usedssd=%ld\n", ssd_strategy_control->n_usedssd, ssd_strategy_control->first_usedssd);
            }
            //分配锁
            pthread_mutex_lock(&free_ssd_mutex);
			interval_time = 0;
			//遍历整个ssd内部，如果ssd_flag标记为可以数据，flush 数据
			for (i = ssd_strategy_control->first_usedssd; i < ssd_strategy_control->first_usedssd + NSSDCLEAN; i++) {
				if (ssd_descriptors[i%NSSDs].ssd_flag & SSD_VALID) {
					flushSSD(&ssd_descriptors[i%NSSDs]);
				}
			}
			//修改位置
			ssd_strategy_control->first_usedssd = (ssd_strategy_control->first_usedssd + NSSDCLEAN) % NSSDs;

			//已经使用的数据需要减去clean的数据量
			ssd_strategy_control->n_usedssd -= NSSDCLEAN;
			//处理完成后releaselock
            pthread_mutex_unlock(&free_ssd_mutex);
            if (DEBUG) 
                printf("[INFO] freeStrategySSD():--------after clean\n");
		}
	}
}

//flush函数，把ssd中的数据flush到底层存储；
/*
一个定义为volatile的变量是说这变量可能会被意想不到地改变，
这样，编译器就不会去假设这个变量的值了。精确地说就是，优
化器在用到这个变量时必须每次都小心地重新读取这个变量的值，
而不是使用保存在寄存器里的备份;

在这里是修饰函数的返回值
这里的作用是帮助编译器进行优化；准确来说就是
但是加了volatile过后，就意味着这个函数不会返回，就相当于
告诉编译器，我调用后是不用保存调用我的函数的返回地址的。
这样就达到了优化的作用。
*/
static volatile void* flushSSD(SSDDesc *ssd_hdr)
{
	long i;
    int returnCode;
    //buffer的大小为4k
	char buffer[BLCKSZ];
	//BNDSZ的大小为8k，char* band[2];
	char* band[BNDSZ/BLCKSZ];
	//定义bool数组；判断使用的数据
	bool bandused[BNDSZ/BLCKSZ];

	unsigned long BandNum = GetSMRBandNumFromSSD(ssd_hdr);
	off_t Offset;

	memset(bandused, 0, BNDSZ/BLCKSZ*sizeof(bool));
	for (i = 0; i < BNDSZ/BLCKSZ; i++)
		band[i] = (char *) malloc(sizeof(char)*BLCKSZ);

	//pread是一个函数，用于带偏移量地原子的从文件中读取数据。
	returnCode = pread(inner_ssd_fd, band[GetSMROffsetInBandFromSSD(ssd_hdr)], BLCKSZ, ssd_hdr->ssd_id * BLCKSZ);
	//如果读数出错，就输出对应的错误信息：fd,errorcode,offset;errorcode 表示pread函数读书出错返回的值；
	if(returnCode < 0) {
		printf("[ERROR] flushSSD():-------read from inner ssd: fd=%d, errorcode=%d, offset=%lu\n", inner_ssd_fd, returnCode, ssd_hdr->ssd_id * BLCKSZ);
        exit(-1);
	}

	//遍历所有sdd中使用的单元；
	for (i = ssd_strategy_control->first_usedssd; i < ssd_strategy_control->first_usedssd+ssd_strategy_control->n_usedssd; i++)
	{
		//ssd_descriptors[i%NSSDs].ssd_flag & SSD_VALID 
		//GetSMRBandNumFromSSD(&ssd_descriptors[i%NSSDs]) == BandNum
		if (ssd_descriptors[i%NSSDs].ssd_flag & SSD_VALID && GetSMRBandNumFromSSD(&ssd_descriptors[i%NSSDs]) == BandNum) {
			//得到当前处理数据在ssd中的偏移量。
			Offset = GetSMROffsetInBandFromSSD(&ssd_descriptors[i%NSSDs]);
			//读取数据
			returnCode = pread(inner_ssd_fd, band[Offset], BLCKSZ, ssd_descriptors[i%NSSDs].ssd_id * BLCKSZ);
			//读取失败，返回
			if(returnCode < 0) {
				printf("[ERROR] flushSSD():-------read from inner ssd: fd=%d, errorcode=%d, offset=%lu\n", inner_ssd_fd, returnCode, ssd_descriptors[i%NSSDs].ssd_id * BLCKSZ);
		                exit(-1);
			}
			//更改标志
			bandused[Offset] = 1;
			//得到hashcode
		    long tmp_hash = ssdtableHashcode(&ssd_descriptors[i%NSSDs].ssd_tag);
		    //得到id号
		    long tmp_id = ssdtableLookup(&ssd_descriptors[i%NSSDs].ssd_tag, tmp_hash);
		    //删除ssdtable
		    ssdtableDelete(&ssd_descriptors[i%NSSDs].ssd_tag, ssdtableHashcode(&ssd_descriptors[i%NSSDs].ssd_tag));
		    //更改标志位
			ssd_descriptors[i%NSSDs].ssd_flag = 0;
		}
	}
	
	//访问各个block；BNDSZ/BLCKSZ为一个band中block的个数
	for (i = 0; i < BNDSZ/BLCKSZ; i++)
	{
		//如果bandused[i] == 0；读数
		if (bandused[i] == 0) {
			returnCode = pread(smr_fd, band[i], BLCKSZ, (BandNum * BNDSZ / BLCKSZ + i) * BLCKSZ);
			if(returnCode < 0) {
				printf("[ERROR] flushSSD():-------read from smr: fd=%d, errorcode=%d, offset=%lu\n", inner_ssd_fd, returnCode, (BandNum * BNDSZ / BLCKSZ + i) * BLCKSZ);
		                exit(-1);
			}
		}
	}
	//把读出来的数写到smr中
	returnCode = pwrite(smr_fd, band, BNDSZ, BandNum * BNDSZ);
	if(returnCode < 0) {
		printf("[ERROR] flushSSD():-------write to smr: fd=%d, errorcode=%d, offset=%lu\n", inner_ssd_fd, returnCode, BandNum * BNDSZ);
                exit(-1);
	}
}

//从SSD得到SMR的band数目；用偏移量/band的大小;
static unsigned long GetSMRBandNumFromSSD(SSDDesc *ssd_hdr)
{
	return ssd_hdr->ssd_tag.offset / BNDSZ;
}


static off_t GetSMROffsetInBandFromSSD(SSDDesc *ssd_hdr)
{
	return (ssd_hdr->ssd_tag.offset / BLCKSZ) % (BNDSZ / BLCKSZ);
}


//把数据写入smr
int smrwrite(int smr_fd, char* buffer, size_t size, off_t offset)
{
	SSDTag ssd_tag;
	SSDDesc *ssd_hdr;
	long i;
    int returnCode;
	long ssd_hash;
	long ssd_id;

	//循环写入数据
	for (i = 0; i * BLCKSZ < size; i++) {
		ssd_tag.offset = offset + i * BLCKSZ;

		//在ssdtable中得到hash和id号
		ssd_hash = ssdtableHashcode(&ssd_tag);
		ssd_id = ssdtableLookup(&ssd_tag, ssd_hash);

		//如果找到id号
		if (ssd_id >= 0) {
			ssd_hdr = &ssd_descriptors[ssd_id];
		}
		else {//否则在使用的ssd链表中查找，并解锁
			ssd_hdr = getStrategySSD();
			//releaselock
            pthread_mutex_unlock(&free_ssd_mutex);
		}


		ssdtableInsert(&ssd_tag, ssd_hash, ssd_hdr->ssd_id);
		ssd_hdr->ssd_flag |= SSD_VALID | SSD_DIRTY;
		ssd_hdr->ssd_tag = ssd_tag;
		
		//写数据
		returnCode = pwrite(inner_ssd_fd, buffer, BLCKSZ, ssd_hdr->ssd_id * BLCKSZ);
		if(returnCode < 0) {
        		printf("[ERROR] smrwrite():-------write to smr disk: fd=%d, errorcode=%d, offset=%lu\n", inner_ssd_fd, returnCode, offset + i * BLCKSZ);
		               exit(-1);
        	}
		
	}

}


static SSDDesc *getStrategySSD()
{
	SSDDesc *ssd_hdr;

	//先判断使用的ssd数目是否大于限定的条件；给出提示条件
	while (ssd_strategy_control->n_usedssd >= NSSDs)
	{
		usleep(1);
		if (DEBUG) printf("[INFO] getStrategySSD():--------ssd_strategy_control->n_usedssd=%ld\n", ssd_strategy_control->n_usedssd);	
	}
	//分配锁
    pthread_mutex_lock(&free_ssd_mutex);

	ssd_strategy_control->last_usedssd = (ssd_strategy_control->last_usedssd + 1) % NSSDs;
	ssd_strategy_control->n_usedssd++;
	
	return &ssd_descriptors[ssd_strategy_control->last_usedssd];
}

//smr read
int smrread(int smr_fd, char* buffer, size_t size, off_t offset)
{
	SSDTag ssd_tag;
	SSDDesc *ssd_hdr;
	long i;
    int returnCode;
	long ssd_hash;
	long ssd_id;
	//i为block的个数，一个一个block的读取

	for (i = 0; i * BLCKSZ < size; i++) {
		//先得到偏移量
		ssd_tag.offset = offset + i * BLCKSZ;
		//根据偏移量求hashcode和ssd id号
		ssd_hash = ssdtableHashcode(&ssd_tag);
		ssd_id = ssdtableLookup(&ssd_tag, ssd_hash);

		//如果找到id号
		if (ssd_id >= 0) {
			ssd_hdr = &ssd_descriptors[ssd_id];
			//在inner_ssd_fd读数，得到返回值
			returnCode = pread(inner_ssd_fd, buffer, BLCKSZ, ssd_hdr->ssd_id * BLCKSZ);
		    
		    if(returnCode < 0) {
        		printf("[ERROR] smrread():-------read from inner ssd: fd=%d, errorcode=%d, offset=%lu\n", inner_ssd_fd, returnCode, ssd_hdr->ssd_id * BLCKSZ);
               	exit(-1);
	        }
			return returnCode;
		} else {
			//否则在smr_fd中读数
			returnCode = pread(smr_fd, buffer, BLCKSZ, offset + i * BLCKSZ);
			if(returnCode < 0) {
        			printf("[ERROR] smrread():-------read from smr disk: fd=%d, errorcode=%d, offset=%lu\n", inner_ssd_fd, returnCode, offset + i * BLCKSZ);
		                exit(-1);
        		}
		}
	}
	return 0;
}





