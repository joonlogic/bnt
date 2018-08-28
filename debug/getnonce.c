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

#define MAX_BLOCKS      0x100000

void show_results(unsigned int* nonces, int count)
{
	int steps = 0;
	int i = 0;
	unsigned long long* pool = NULL;
	unsigned int* noncep = nonces;

	printf("* NONCE distibution ---\n");
#if VERBOSE
	printf("* Enter steps : ");
	scanf("%d", &steps);
#else
	for(steps=2; steps<=256; steps<<=1) {
	printf("* Steps : %d\n", steps);
#endif

	unsigned int unit_nonce = 
		(unsigned int)(0x100000000ULL / (unsigned long long)steps);
	printf("* Unit nonce %08X\n", unit_nonce);

	pool = malloc(steps*sizeof(unsigned long long));
	if(!pool) {
		printf("Error malloc()\n");
		return;
	}
	memset(pool, 0x00, steps*sizeof(unsigned long long));

	unsigned int pond = 0;
	do {
		pond = *noncep++ / unit_nonce;
		pool[pond]++;
	} while(++i < count);

	for(i=0; i<steps; i++) {
		printf("pool[%d] ( %08x ~ %08x ) : %llu\n", 
				i, i*unit_nonce, (i+1)*unit_nonce-1, pool[i]);
	}
	printf("\n");
	free(pool);
#if VERBOSE
#else
	noncep = nonces;
	i = 0;
	}
#endif
}

int main(void)
{
	FILE* fp = NULL;
	fp = fopen("./blockchain_headers", "r");
	if(!fp) {
		printf("Error opening file\n");
		return -1;
	}

	BlockHeader_t bheader = {0,}; 
	int count = 0;
	int readlen = 0;
	unsigned int nonces[MAX_BLOCKS] = {0,};
	unsigned int* noncep = nonces;
	time_t ntime;

	do {
		readlen = fread((void*)&bheader, sizeof(bheader), 1, fp);
		*noncep++ = bheader.nonce; 

		if(count % 10000 == 0)  {
			ntime = bheader.timestamp;
			printf("[%d] NONCE %08x     %s", count, 
					bheader.nonce, ctime(&ntime));
		}
		count++;

	}while(readlen);

	ntime = bheader.timestamp;
	printf("===== TOTAL %d block headers LAST %s\n", count, ctime(&ntime));

	fclose(fp);

	show_results(nonces, count);

	return 0;
} 
