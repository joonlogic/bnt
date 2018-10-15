#include <stdio.h>
#include <arpa/inet.h>

void bnt_str2hex(
        char* str,
        int len,
        unsigned char* hex
        )
{
    do {
        sscanf(str, "%2hhx", hex++);
        str += 2;
    } while(*str);
}

unsigned int bnt_get_bits(
        unsigned char* hash
        )
{
//input hash is hex format
    int zerobyte = 0;
    unsigned int bits_0 = 0;
    unsigned int bits = 0;

    do {
        if(hash[zerobyte]) break;
    } while(zerobyte++ < 32);

    if(zerobyte == 32) {
        printf("%s: Something Wrong. zerobyte %d\n", __func__, zerobyte);
        return 0x7FFFFFFF;
    }

    bits_0 = 32 - zerobyte;
    bits = ( bits_0 << 24 ) | ((ntohl(*(unsigned int*)&hash[zerobyte]))>>8);

    printf("%s: MY BITS ARE %08X\n", __func__, bits);

    return bits;
}

int main(int argc, char* argv[])
{
	char* hashstr = argv[1];
	unsigned int bits = 0;

	unsigned char hashout[32] = {0,};

	if(!(hashstr)) {
		printf("Please input hash string...\n");
		return 0;
	}

	bnt_str2hex(hashstr, 64, hashout);
	bits = bnt_get_bits(hashout);

	printf("bits = %08X\n", bits);

	return 0;
}
