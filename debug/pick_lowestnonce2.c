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

//Pick 2 samples
//Upper 9bits should be non-zero

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
	unsigned int picknonce = 0;
	unsigned int lowernonce = 0xFFFFFFFF;
	BlockHeader_t lbh = {0,};
	BlockHeader_t lbh2nd = {0,};
	int lowcount = 0;
	int lowcount2nd = 0;

	do {
		readlen = fread((void*)&bheader, sizeof(bheader), 1, fp);
		picknonce = ntohl(bheader.nonce);
		if(picknonce & 0xF8000000) continue;
		picknonce &= 0x007FFFFF;

		if(bheader.nonce && (picknonce < lowernonce)) {
			lowernonce = picknonce;
			memcpy(&lbh2nd, &lbh, sizeof(bheader));
			memcpy(&lbh, &bheader, sizeof(bheader));
			printf("COUNT %d NONCE %08X\n", count, lowernonce);
			lowcount2nd = lowcount;
			lowcount = count;
		}
	}while(readlen && ++count);

	//For sample file
	char buf[64] = {0,};
	//Lowest
	sprintf(buf, "BH_LOWNONCE_23b[0]_%d.bin", lowcount);
	FILE* samplefp = fopen(buf, "w+");
	if(!samplefp) {
		printf("%s: Error opening file 2\n", __func__);
		return -1;
	}

	printf("LOWEST NONCE %08X\n", ntohl(lbh.nonce));
	fwrite(&lbh, sizeof(lbh), 1, samplefp);
	fclose(samplefp);

	//2nd Lowest
	sprintf(buf, "BH_LOWNONCE_23b[1]_%d.bin", lowcount2nd);
	samplefp = fopen(buf, "w+");
	if(!samplefp) {
		printf("%s: Error opening file 3\n", __func__);
		return -1;
	}

	printf("2nd LOWEST NONCE %08X\n", ntohl(lbh2nd.nonce));
	fwrite(&lbh2nd, sizeof(lbh2nd), 1, samplefp);
	fclose(samplefp);

	printf("DONE...\n");
	fclose(fp);

	return 0;
} 
