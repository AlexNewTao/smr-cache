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


}