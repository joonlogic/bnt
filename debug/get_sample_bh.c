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

int main(void)
{
	int steps = 0;
	int i = 0;
	unsigned long long* pool = NULL;

	FILE* fp = NULL;
	fp = fopen("./blockchain_headers", "r");
	if(!fp) {
		printf("Error opening file\n");
		return -1;
	}

	BlockHeader_t bheader = {0,}; 
	int count = 0;
	int readlen = 0;
	time_t ntime;
	unsigned int pond = 0;

	for(steps=2; steps<=256; steps<<=1) {
		printf("* Steps : %d\n", steps);
		unsigned int unit_nonce = 
			(unsigned int)(0x100000000ULL / (unsigned long long)steps);
		printf("* Unit nonce %08X\n", unit_nonce);

		pool = malloc(steps*sizeof(unsigned long long));
		if(!pool) {
			printf("Error malloc()\n");
			return -1;
		}
		memset(pool, 0x00, steps*sizeof(unsigned long long));

		//For sample file
		char buf[64] = {0,};
		sprintf(buf, "BNT_BH_%03d.bin", steps);
		FILE* samplefp = fopen(buf, "w+");
		if(!samplefp) {
			printf("%s: Error opening file\n", __func__);
			return -1;
		}

		fseek( fp, sizeof(bheader)*450000, SEEK_SET ); //skip to Year 2017

		do {
			readlen = fread((void*)&bheader, sizeof(bheader), 1, fp);

			pond = bheader.nonce / unit_nonce;
			pool[pond]++;
			if(pool[pond]==1) {
				fwrite(&bheader, sizeof(bheader), 1, samplefp);
				ntime=bheader.timestamp;
				printf(" - %s pool[%d] : nonce %08X %s", 
						buf, pond, bheader.nonce, ctime(&ntime));
			}
		} while(readlen);

/*
		for(i=0; i<steps; i++) {
			printf("pool[%d] ( %08x ~ %08x ) : %llu\n", 
					i, i*unit_nonce, (i+1)*unit_nonce-1, pool[i]);
		}
*/
		printf("\n");
		free(pool);
		fclose(samplefp);
	}

	fclose(fp);

	return 0;
} 
