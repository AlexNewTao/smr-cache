





//
typedef struct
{
        SSDTag     ssd_tag;
        long       ssd_id;			// ssd buffer location 
        unsigned   ssd_flag;
//	long		usage_count;
//	long		next_freessd;
} SSDDesc;

//定义结构体 ssd控制策略，在这里有clock和lru两种策略
typedef struct
{
	unsigned long		n_usedssd;  //使用的ssd数目
	long		first_usedssd;		// Head of list of used ssds
	long		last_usedssd;		// Tail of list of used ssds
} SSDStrategyControl;


//定义SSDHashBucket结构体，两个成员变量和一个指针
typedef struct SSDHashBucket
{
        SSDTag				hash_key;
        long                             ssd_id;
        struct SSDHashBucket		*next_item;
} SSDHashBucket;
