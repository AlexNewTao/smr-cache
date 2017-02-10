



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

	ssd_blocks = (char *) malloc(SSD_SIZE*NSSDs);
	memset(ssd_blocks, 0, SSD_SIZE*NSSDs);

    pthread_mutex_init(&free_ssd_mutex, NULL);
//    pthread_mutex_init(&inner_ssd_hdr_mutex, NULL);
//    pthread_mutex_init(&inner_ssd_table_mutex, NULL);
	int err;

    err = pthread_create(&freessd_tid, NULL, freeStrategySSD, NULL);
    if (err != 0) {
        printf("[ERROR] initSSD: fail to create thread: %s\n", strerror(err));
    }
}




