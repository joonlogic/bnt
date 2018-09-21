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


static void
bnt_fill_zeros(unsigned int nbytes, char* outstr)
{
    //outstr should be clean
    for(int i=0; i<nbytes; i++) {
        *outstr++ = '0';
        *outstr++ = '0';
    }
}

static void
bnt_pad_zeros(char* outstr)
{
    for(int i=strlen(outstr); i<64; i++) {
       outstr[i] = '0';
    }
}

void bnt_get_targetstr(
        unsigned int bits,
        char* str
        )
{
    unsigned int nzerobytes = ((bits >> 24) - 3);
    unsigned int target_int = bits & 0x00FFFFFF;

    bnt_fill_zeros(32 - (3 + nzerobytes), str); //6 means strlen of target_int
    sprintf(str + strlen(str), "%06X", target_int);
    bnt_pad_zeros(str);
}

int main(void)
{
	int steps = 0;
	int i = 0;
	unsigned int bits = 0;

	printf("Input bits in hex form : ");
	scanf("%X", &bits);

	printf("\nBITS : %08X\n", bits);

	char targetstr[65] = {0,};
	bnt_get_targetstr(bits, targetstr);

	printf("Target : %s\n", targetstr); 
	return 0;
} 
