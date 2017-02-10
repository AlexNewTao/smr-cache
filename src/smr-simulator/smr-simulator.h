



//定义结构体 ssd控制策略，在这里有clock和lru两种策略
typedef struct
{
	unsigned long		n_usedssd;  //使用的ssd数目
	long		first_usedssd;		// Head of list of used ssds
	long		last_usedssd;		// Tail of list of used ssds
} SSDStrategyControl;