#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <openssl/sha.h>

typedef struct block_header {
	unsigned int    version;
	unsigned char   prev_block[32];
	unsigned char   merkle_root[32];
	unsigned int    timestamp;
	unsigned int    bits;
	unsigned int    nonce;
} BlockHeader_t;

bool 
bnt_gethash(
		unsigned char* input,
		unsigned int length,
		unsigned char* out
		)
{
	SHA256_CTX context;
	if(!SHA256_Init(&context)) return false;
	if(!SHA256_Update(&context, input, length)) return false;
	if(!SHA256_Final(out, &context)) return false;

	return true;
}

void 
bnt_hash2str(
		unsigned char* hash,
		char* out
		)
{
	static const char* tbl = "0123456789abcdef";
	for(int i=0; i<SHA256_DIGEST_LENGTH; i++) {
	out[2*i+0] = tbl[hash[i] >> 4];
	out[2*i+1] = tbl[hash[i] & 0x0F];
	}
}

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

	BlockHeader_t bh = {0,}; 
	int count = 0;
	int readlen = 0;
	time_t ntime;
	unsigned char hashout[32] = {0,};
	char strhashout[65] = {0,};
	bool ret = false;

	do {
		readlen = fread((void*)&bh, sizeof(bh), 1, fp);
		ret = bnt_gethash((unsigned char*)&bh, sizeof(bh), hashout);
		if(ret==false) { printf("%d [%d] return false\n", __LINE__, count);break; }
		ret = bnt_gethash(hashout, sizeof(hashout), hashout);
		if(ret==false) { printf("%d [%d] return false\n", __LINE__, count);break; }

		bnt_hash2str(hashout, strhashout);
		ntime = bh.timestamp;
		printf("[%d] %s HASHOUT : %s\n", count, ctime(&ntime), strhashout);

		count++;

	} while(readlen);

	printf("\n");
	fclose(fp);

	return 0;
} 
