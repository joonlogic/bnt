#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

typedef struct block_header {
	unsigned int    version;
	unsigned char   prev_block[32];
	unsigned char   merkle_root[32];
	unsigned int    timestamp;
	unsigned int    bits;
	unsigned int    nonce;
} BlockHeader_t;

unsigned int rand_interval(unsigned int min, unsigned int max)
{
    int r;
    const unsigned int range = 1 + max - min;
    const unsigned int buckets = RAND_MAX / range;
    const unsigned int limit = buckets * range;

    /* Create equal size buckets all in a row, then fire randomly towards
     * the buckets until you land in one of them. All buckets are equally
     * likely. If you land off the end of the line of buckets, try again. */
    do
    {
        r = rand();
    } while (r >= limit);

    return min + (r / buckets);
}

int main(void)
{
	unsigned int howmany = 0;

	printf("How many samples do you want? ( Max 400000 ) : ");
	scanf("%d", &howmany);
	puts("");

	howmany = howmany > 400000 ? 400000 : howmany;

	FILE* fp = NULL;
	fp = fopen("./blockchain_headers", "r");
	if(!fp) {
		printf("Error opening file 1\n");
		return -1;
	}

	BlockHeader_t bheader = {0,}; 
	int readlen = 0;
	time_t ntime;

	//For sample file
	char buf[64] = {0,};
	sprintf(buf, "BH_RAND_%d.bin", howmany);
	FILE* samplefp = fopen(buf, "w+");
	if(!samplefp) {
		printf("%s: Error opening file 2\n", __func__);
		return -1;
	}

	int count = 1;

	unsigned int min = 0;
	unsigned int max = 477636;
	int id = 0;

	srand(time(NULL));

	do {
		id = rand_interval(min, max);
		fseek(fp, sizeof(bheader)*id, SEEK_SET); //skip to the id

		readlen = fread((void*)&bheader, sizeof(bheader), 1, fp);
		fwrite(&bheader, sizeof(bheader), 1, samplefp);

		printf("[%d] Sample id %d picked\n", count, id);
	} while(count++ < howmany);

	printf("DONE...\n");
	printf("FILENAME : %s\n", buf);

	fclose(samplefp);
	fclose(fp);

	return 0;
} 
