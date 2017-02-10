



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <memory.h>





























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
			for (i = ssd_strategy_control->first_usedssd; i < ssd_strategy_control->first_usedssd + NSSDCLEAN; i++) {
				if (ssd_descriptors[i%NSSDs].ssd_flag & SSD_VALID) {
					flushSSD(&ssd_descriptors[i%NSSDs]);
				}
			}
			ssd_strategy_control->first_usedssd = (ssd_strategy_control->first_usedssd + NSSDCLEAN) % NSSDs;
			ssd_strategy_control->n_usedssd -= NSSDCLEAN;
			//releaselock
            pthread_mutex_unlock(&free_ssd_mutex);
            if (DEBUG) 
                printf("[INFO] freeStrategySSD():--------after clean\n");
		}
	}
}


static volatile void* flushSSD(SSDDesc *ssd_hdr)
{
	long i;
    int returnCode;
	char buffer[BLCKSZ];
	char* band[BNDSZ/BLCKSZ];
	bool bandused[BNDSZ/BLCKSZ];
	unsigned long BandNum = GetSMRBandNumFromSSD(ssd_hdr);
	off_t Offset;

	memset(bandused, 0, BNDSZ/BLCKSZ*sizeof(bool));
	for (i = 0; i < BNDSZ/BLCKSZ; i++)
		band[i] = (char *) malloc(sizeof(char)*BLCKSZ);
	returnCode = pread(inner_ssd_fd, band[GetSMROffsetInBandFromSSD(ssd_hdr)], BLCKSZ, ssd_hdr->ssd_id * BLCKSZ);
	if(returnCode < 0) {
		printf("[ERROR] flushSSD():-------read from inner ssd: fd=%d, errorcode=%d, offset=%lu\n", inner_ssd_fd, returnCode, ssd_hdr->ssd_id * BLCKSZ);
        exit(-1);
	}

	for (i = ssd_strategy_control->first_usedssd; i < ssd_strategy_control->first_usedssd+ssd_strategy_control->n_usedssd; i++)
	{
		if (ssd_descriptors[i%NSSDs].ssd_flag & SSD_VALID && GetSMRBandNumFromSSD(&ssd_descriptors[i%NSSDs]) == BandNum) {
			Offset = GetSMROffsetInBandFromSSD(&ssd_descriptors[i%NSSDs]);
			returnCode = pread(inner_ssd_fd, band[Offset], BLCKSZ, ssd_descriptors[i%NSSDs].ssd_id * BLCKSZ);
			if(returnCode < 0) {
				printf("[ERROR] flushSSD():-------read from inner ssd: fd=%d, errorcode=%d, offset=%lu\n", inner_ssd_fd, returnCode, ssd_descriptors[i%NSSDs].ssd_id * BLCKSZ);
		                exit(-1);
			}
			bandused[Offset] = 1;
		    long tmp_hash = ssdtableHashcode(&ssd_descriptors[i%NSSDs].ssd_tag);
		    long tmp_id = ssdtableLookup(&ssd_descriptors[i%NSSDs].ssd_tag, tmp_hash);
		    ssdtableDelete(&ssd_descriptors[i%NSSDs].ssd_tag, ssdtableHashcode(&ssd_descriptors[i%NSSDs].ssd_tag));
			ssd_descriptors[i%NSSDs].ssd_flag = 0;
		}
	}
	
	for (i = 0; i < BNDSZ/BLCKSZ; i++)
	{
		if (bandused[i] == 0) {
			returnCode = pread(smr_fd, band[i], BLCKSZ, (BandNum * BNDSZ / BLCKSZ + i) * BLCKSZ);
			if(returnCode < 0) {
				printf("[ERROR] flushSSD():-------read from smr: fd=%d, errorcode=%d, offset=%lu\n", inner_ssd_fd, returnCode, (BandNum * BNDSZ / BLCKSZ + i) * BLCKSZ);
		                exit(-1);
			}
		}
	}

	returnCode = pwrite(smr_fd, band, BNDSZ, BandNum * BNDSZ);
	if(returnCode < 0) {
		printf("[ERROR] flushSSD():-------write to smr: fd=%d, errorcode=%d, offset=%lu\n", inner_ssd_fd, returnCode, BandNum * BNDSZ);
                exit(-1);
	}
}


