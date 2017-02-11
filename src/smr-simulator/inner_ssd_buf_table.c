


void initSSDTable(size_t size)
{
	ssd_hashtable = (SSDHashBucket *)malloc(sizeof(SSDHashBucket)*size);
	size_t i;
	SSDHashBucket *ssd_hash = ssd_hashtable;
	for (i = 0; i < size; ssd_hash++, i++){
		ssd_hash->ssd_id = -1;
		ssd_hash->hash_key.offset = -1;
		ssd_hash->next_item = NULL;
	}
}

//得到ssdtable的hashcode
unsigned long ssdtableHashcode(SSDTag *ssd_tag)
{
	unsigned long ssd_hash = (ssd_tag->offset / SSD_BUFFER_SIZE) % NSSDTables;
	return ssd_hash;
}


long ssdtableLookup(SSDTag *ssd_tag, unsigned long hash_code)
{
	if (DEBUG)
		printf("[INFO] Lookup ssd_tag: %lu\n",ssd_tag->offset);
	SSDHashBucket *nowbucket = GetSSDHashBucket(hash_code);
	while (nowbucket != NULL) {
	//	printf("nowbucket->buf_id = %u %u %u\n", nowbucket->hash_key.rel.database, nowbucket->hash_key.rel.relation, nowbucket->hash_key.block_num);
		if (isSamessd(&nowbucket->hash_key, ssd_tag)) {
	//		printf("find\n");
			return nowbucket->ssd_id;
		}
		nowbucket = nowbucket->next_item;
	}
//	printf("no find\n");

	return -1;
}

