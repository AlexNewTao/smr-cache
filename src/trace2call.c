#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "ssd-cache.h"
#include "smr-simulator/smr-simulator.h"


//把文件的数据读出来
void trace_to_iocall(char* trace_file_path) {
	//定义文件句柄
	FILE* trace;
	//打开文件，并判断错误
	if((trace = fopen(trace_file_path, "rt")) == NULL) {
		printf("[ERROR] trace_to_iocall():--------Fail to open the trace file!");
		exit(1);
	}

	double time, time_begin, time_now;

	/*
	该结构体是Linux系统中定义，struct timeval结构体在time.h中的定义为：

	struct timeval
	{
	__time_t tv_sec;        // Seconds. 
	__suseconds_t tv_usec;  // Microseconds.
	};
	*/
    struct timeval tv_begin, tv_now;

    /*
    struct timezone结构的定义为: 
	struct timezone
	{
   		int tz_minuteswest; // 和Greewich时间差了多少分钟
   		int tz_dsttime; // 日光节约时间的状态 
	};
    */
    struct timezone tz_begin, tz_now;

	long time_diff;
	char action;

	char write_or_read[100];

	off_t offset;
    size_t size;
	char* ssd_buffer;
	bool is_first_call = 1;
	int i;

	// int gettimeofday(struct  timeval*tv,struct  timezone *tz )
	//gettimeofday()会把目前的时间用tv 结构体返回，当地时区的信息则放到tz所指的结构中
    gettimeofday(&tv_begin, &tz_begin);

    //得到开始时间
    time_begin = tv_begin.tv_sec + tv_begin.tv_usec/1000000.0;

    //循环读数据
    while(!feof(trace)) {
    	//从trace流中执行格式化输入
		fscanf(trace, "%lf %c %s %lu %lu", &time, &action, write_or_read, &offset, &size);

        gettimeofday(&tv_now, &tz_now);
        
        if (DEBUG)
            printf("[INFO] trace_to_iocall():--------now time = %lf\n", time_now-time_begin);

        //得到当前时间
        time_now = tv_now.tv_sec + tv_now.tv_usec/1000000.0;
        //判断是否是第一次调用
		if (!is_first_call) {
			time_diff = (time - (time_now - time_begin)) * 1000000;
			if (time_diff > 0)
                usleep(time_diff);
		} else {
			is_first_call = 0;
		}

        size = size*1024;

        //strstr(str1,str2) 函数用于判断字符串str2是否是str1的子串。如果是，则该函数返回str2在str1中首次出现的地址；否则，返回NULL。
        //在这里判断是读操作还是写操作
		if(strstr(write_or_read, "W")) {
			//分配空间
            ssd_buffer = (char *)malloc(sizeof(char)*BLCKSZ);

            //初始化ssd_buffer标志位
            for (i=0; i<BLCKSZ; i++)
                ssd_buffer[i] = '1';

            while (size > 0 ) {
                if (DEBUG)
                    printf("[INFO] trace_to_iocall():--------wirte offset=%lu\n", offset);

			    write_block(offset, ssd_buffer);
                offset += BLCKSZ;
                size -= BLCKSZ;
            }
		} else if(strstr(write_or_read, "R")) {
			//如果是读操作
		    if (DEBUG)
               	printf("[INFO] trace_to_iocall():--------read offset=%lu\n", offset);
			read_block(offset, ssd_buffer); 
		}
	}

	//记录时间
    gettimeofday(&tv_now, &tz_now);

    time_now = tv_now.tv_sec + tv_now.tv_usec/1000000.0;

    printf("total run time (s) = %lf\n", time_now - time_begin);

	fclose(trace);
	
}



