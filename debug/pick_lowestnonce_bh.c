#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
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
	unsigned int count = 0;

	/*
	printf("Input ID : \n");
	scanf("%d", &id);
	*/

	FILE* fp = NULL;
	fp = fopen("./blockchain_headers", "r");
	if(!fp) {
		printf("Error opening file 1\n");
		return -1;
	}

	BlockHeader_t bheader = {0,}; 
	int readlen = 0;
	time_t ntime;

//	fseek( fp, sizeof(bheader)*id, SEEK_SET ); //skip to Year 2017
	unsigned int nonce = 0;
	unsigned int lowernonce = 0xFFFFFFFF;
	BlockHeader_t lbh = {0,};
	int lowcount = 0;

	do {
		readlen = fread((void*)&bheader, sizeof(bheader), 1, fp);
		if(bheader.nonce && (ntohl(bheader.nonce) < lowernonce)) {
			lowernonce = ntohl(bheader.nonce);
			memcpy(&lbh, &bheader, sizeof(bheader));
			printf("COUNT %d NONCE %08X\n", count, lowernonce);
			lowcount = count;
		}
		count++;
	}while(readlen);

	//For sample file
	char buf[64] = {0,};
	sprintf(buf, "BH_LOWNONCE_%d.bin", lowcount);
	FILE* samplefp = fopen(buf, "w+");
	if(!samplefp) {
		printf("%s: Error opening file 2\n", __func__);
		return -1;
	}

	printf("LOWEST NONCE %08X\n", ntohl(lbh.nonce));
	fwrite(&lbh, sizeof(lbh), 1, samplefp);

	printf("DONE...\n");
	fclose(samplefp);
	fclose(fp);

	return 0;
} 
