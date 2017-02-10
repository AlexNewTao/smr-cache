


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