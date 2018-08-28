#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

void str2hex(char* str, int len, unsigned char* bin)
{
	do {
		sscanf(str, "%2hhx", bin++);
		str += 2;
	} while(*str);
}

void hex2str(unsigned char* hash, char* outputBuffer)
{
	int i = 0;

	for(i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
	}
}

static void swap256(void *dest_p, const void *src_p)
{
	unsigned char *dest = dest_p;
	const unsigned char *src = src_p;

	for(int i=0; i<32; i++) {
		dest[i] = src[32-i-1];
	}
}

int main()
{
#if 0
    char* sin = "00000020" // version
        "164a1e4a7f34b96b0e201dcc6a623c63fe3874696e4875000000000000000000" // prev hash
        "49de8b4f4bfa9fc890d3d28a93156a111f891dc680090cd497b58a7d5c2b09cf" // merkle
        "2f62345a"      // timestamp
        "edb00018"      // bits
        "ffdfd257";
#else
	char* sin = "01000000" //version
		"0000000000000000000000000000000000000000000000000000000000000000" //prev hash
		"3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a" //merkle
		"29ab5f49" 
		"ffff001d" 
		"1dac2b7c"; 
#endif

	unsigned char hashout[SHA256_DIGEST_LENGTH]={0,};
	unsigned char bin[128]={0,};
	int lenstr = strlen(sin);

	printf("length = %d\n", lenstr);
	printf("sin = %s\n", sin);
	str2hex(sin, lenstr, bin);


	/*
	unsigned char swaper[32]={0,};
	memcpy(swaper, &bin[36], 32);
	swap256(&bin[36], swaper);
	*/

	//Hashing
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, bin, lenstr/2);
    SHA256_Final(hashout, &ctx);

    SHA256_Init(&ctx);
    SHA256_Update(&ctx, hashout, SHA256_DIGEST_LENGTH);
    SHA256_Final(hashout, &ctx);

#if 1

    const char * tbl = "0123456789abcdef";
    char outs[64 + 1] = { 0 };
    for (int i = 0; i < 32; ++i)
    {
        outs[2*i+0] = tbl[hashout[31-i] >> 4];
        outs[2*i+1] = tbl[hashout[31-i] & 0xf];
    }
    printf("%s\n", outs);

#else

	char outs[128]={0,};
	hex2str(hashout, outs);
    printf("%s\n", outs);

#endif

    return 0;
}
