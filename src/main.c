/*
    use hdd simulator ssd and smr

    main function

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "main.h"
#include "ssd-cache.h"
#include "smr-simulator/smr-simulator.h"
#include "trace2call.h"

int main()
{
	//指定trace文件的路径
	char trace_file_path[]="../trace_for_test.txt";
	//初始化ssd参数
	initSSD();
	//初始化ssdbuffer
	initSSDBuffer();

	//smr文件描述符；O_RDWR 以可读写方式打开文件；O_SYNC 以同步的方式打开文件.
	smr_fd = open(smr_device, O_RDWR|O_SYNC);

	//ssd文件描述符；O_RDWR 以可读写方式打开文件；O_SYNC 以同步的方式打开文件.
    ssd_fd = open(ssd_device, O_RDWR|O_SYNC);

    //inner ssd文件描述符；O_RDWR 以可读写方式打开文件；O_SYNC 以同步的方式打开文件.
    inner_ssd_fd = open(inner_ssd_device, O_RDWR|O_SYNC);

    //io调用文件
    trace_to_iocall(trace_file_path);

    




}