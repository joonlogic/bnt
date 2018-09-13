#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <openssl/sha.h>
#include "bnt_def.h"
#include "bnt_ext.h"

typedef struct local_block_header {
	unsigned int    version;
	unsigned char   prev_block[32];
	unsigned char   merkle_root[32];
	unsigned int    timestamp;
	unsigned int    bits;
	unsigned int    nonce;
} BlockHeader_t;

typedef struct {
    char*          optstr;
	bool           isfile; 
} T_OptInfo;

static void print_usage(const char *prog)
{
    printf("Usage: %s [-f] [filename] [-s] [hash string] \n", prog);
    puts("  -f --filename Binary file name to be hashed\n"
         "  -s --string   ASCII string to be hashed\n");
    exit(1);
}

static int parse_opts(int argc, char *argv[], T_OptInfo* info)
{
    while (1) {
        static const struct option lopts[] = {
            { "filename", 1, 0, 'f' },
            { "string ",  1, 0, 's' },
            { "help",     0, 0, 'h' },
            { NULL,       0, 0, 0 },
        };
        int c;

        c = getopt_long(argc, argv, "f:s:h", lopts, NULL);

        if (c == -1)
            break;

        switch (c) {
        case 'f':
			info->isfile = true;
            info->optstr = optarg;
			break;
        case 's':
            info->optstr = optarg;
            break;
        case 'h':
        default:
            print_usage(argv[0]);
            break;
        }
    }

    return 0;
}

int readstr(char* str, BlockHeader_t* bh, int count)
{
	if(count) return 0;

	bnt_str2hex(str, 0, (unsigned char*)bh);
	return 1;
}

int main(int argc, char *argv[])
{
    int res = 0;
    T_OptInfo info = {
    };
	FILE* fp = NULL;

    res = parse_opts(argc, argv, &info);
    BNT_CHECK_RESULT(res, res);

	if(info.optstr) {
		if(info.isfile) {
			fp = fopen(info.optstr, "r");
			if(!fp) {
				printf("Error opening file %s\n", info.optstr);
				return -1;
			}
		}
		else {
		}
	}
	else {
		printf("Error. No information input to be hashed.\n");
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
		if(info.isfile) readlen = fread((void*)&bh, sizeof(bh), 1, fp);
		else readlen = readstr(info.optstr, &bh, count);

		if(readlen <= 0) break;

		ret = bnt_gethash((unsigned char*)&bh, sizeof(bh), hashout);
		if(ret==false) { printf("%d [%d] return false\n", __LINE__, count);break; }
		ret = bnt_gethash(hashout, sizeof(hashout), hashout);
		if(ret==false) { printf("%d [%d] return false\n", __LINE__, count);break; }

		bnt_hash2str(hashout, strhashout);
		ntime = bh.timestamp;
		printf("[%d] %s HASHOUT : %s\n", count, ctime(&ntime), strhashout);

		count++;

	} while(1);

	printf("\n");
	if(info.isfile) fclose(fp);

	return 0;
} 
