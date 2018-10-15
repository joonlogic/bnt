#include <stdio.h>
#include <string.h>

#define MIN_S 0
#define MAX_S 6
#define MIN_M 64
#define MAX_M 1023
#define MIN_P 1
#define MAX_P 63
#define MIN_F_REF 2000000.0    //2MHz
#define MAX_F_REF 8000000.0    //8MHz

#define MAX_F_OUT 1200000000.0 //1.2GHz
#define MIN_F_OUT 100000000.0 //100MHz

typedef enum state {
	OUTOF_F_REF,
	UNDER_F_OUT,
	VALID_F_OUT,
	OVER_F_OUT
} PLL_CALC_STATE;

typedef struct f {
	unsigned int s;
	unsigned int p;
	unsigned int m;
	double f_ref;
	double f_out;
} T_F;
	
void printout(T_F* min, T_F*max);

int main(void)
{
	unsigned int s;
	unsigned int p;
	unsigned int m;
	double f_ref = 0.0;
	double f_out = 0.0;
	double f_step = 0.0;
	static const unsigned int f_in[5] = {10000000, 20000000, 25000000, 40000000, 50000000};

	PLL_CALC_STATE state = OUTOF_F_REF;
	T_F f_min = {0,};
	T_F f_max = {0,};

	int i = 0;

	do {
		state = OUTOF_F_REF;
		f_out = 0.0;

		printf("\nF_IN : %f MHz\n", (float)f_in[i]/1000000.);
		printf("S[2:0] | P[5:0] |   M[9:0]   | F_ref  |   F_out (MHz)  |  STEP \n");
		printf("------------------------------------------------------------------\n");

		for(s=MIN_S; s<=MAX_S; s++) {
			for(p=MIN_P; p<=MAX_P; p++) {
				memset(&f_min, 0x00, sizeof(f_min));
				memset(&f_max, 0x00, sizeof(f_max));
				for(m=MIN_M; m<=MAX_M; m++) {
					f_ref = (double)f_in[i]/(double)p;
					if((f_ref < MIN_F_REF) || (f_ref > MAX_F_REF)) continue;
					else f_out = (double)m * f_ref / (double)(1<<s);

					if(f_out == MIN_F_OUT) {
						f_min.s = s;
						f_min.p = p;
						f_min.m = m;
						f_min.f_ref = f_ref;
						f_min.f_out = MIN_F_OUT;
					}
					else if(f_out == MAX_F_OUT) {
						if(f_min.f_out == MIN_F_OUT) {
							f_max.s = s;
							f_max.p = p;
							f_max.m = m;
							f_max.f_ref = f_ref;
							f_max.f_out = MAX_F_OUT;

							printout(&f_min, &f_max); 
							break;
						}
					}
				}
			}
		}
	} while(++i < 5);

	printf("-------------------------------------------------------------------\n\n");

	return 0;
} 

void printout(T_F* min, T_F*max)
{
	printf("%6d | ", min->s);
	printf("%6d | ", min->p);
	printf("%3d ~ %-4d | ", min->m, max->m);
	printf("%4.2f M | ", min->f_ref/1000000.);
	printf("%5.1f ~ %-5.1f | ", min->f_out/1000000., max->f_out/1000000.);
	printf("%4.2f M \n", (max->f_out - min->f_out)/(double)(max->m - min->m)/1000000.);

}
