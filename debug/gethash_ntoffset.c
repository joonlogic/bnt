#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
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
	unsigned int id = 0;

	printf("Input ID : ");
	scanf("%d", &id);

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

	fseek( fp, sizeof(BlockHeader_t)*id, SEEK_SET ); 
	readlen = fread((void*)&bh, sizeof(bh), 1, fp);
	fclose(fp);

	unsigned int nt_org = bh.timestamp;
	unsigned int nonceout = 0;
	unsigned int ntoffset = 0;

	nt_org -= 6; //customized ntoffset temporarily

	do {
		printf("[%d] Input ntime offset : ", count);
		scanf("%d", &ntoffset);

		bh.timestamp = nt_org + ntoffset;
		
		printf("[%d] Input nonce : ", count);
		scanf("%x", &nonceout);
		bh.nonce = htonl(nonceout);

		printf("[%d] ntime %08x, nonce %08x\n", count, bh.timestamp, bh.nonce);

		ret = bnt_gethash((unsigned char*)&bh, sizeof(bh), hashout);
		if(ret==false) { printf("%d [%d] return false\n", __LINE__, count);break; }
		ret = bnt_gethash(hashout, sizeof(hashout), hashout);
		if(ret==false) { printf("%d [%d] return false\n", __LINE__, count);break; }

		bnt_hash2str(hashout, strhashout);
		ntime = bh.timestamp;
		printf("[%d] %s [%d] HASHOUT : %s\n", count, ctime(&ntime), count, strhashout);

		count++;
		printf("\n");

	} while(1);

	printf("\n");

	return 0;
}

