void bnt_swap_str(
        char* instr,
        char* outstr,
        int   sizebyte
        )
{
    if(!instr) return;
    if(!outstr) return;

    instr = instr + sizebyte - 1;
    do {
		printf("sizebyte %d\n", sizebyte);
        *outstr++ = *instr--;
    } while(--sizebyte > 0);
}

int main(void) 
{
	char instr[] = "1234567890";
	char outstr[11] = {0,};

	bnt_swap_str(instr, outstr, 10);

	printf("instr  %s\n", instr);
	printf("outstr %s\n", outstr);

	return 0;
} 
