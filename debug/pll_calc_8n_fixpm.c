#include <stdio.h>
#include <string.h>

#define MIN_S 0
#define MAX_S 6
#define MIN_M 64
#define MAX_M 1023
#define MIN_P 1
#define MAX_P 63
#define MIN_F_REF 4500000.0    //4.5MHz
#define MAX_F_REF 12000000.0    //12MHz

#define MAX_F_OUT 4500000000.0 //4.5GHz
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
void printout_fout(double f_out);

int main(void)
{
	int s;
	int p;
	int m;
	double f_ref = 0.0;
	double f_out = 0.0;
	double f_step = 0.0;
	static const unsigned int f_in[5] = {10000000, 20000000, 25000000, 40000000, 50000000};

	PLL_CALC_STATE state = OUTOF_F_REF;
	T_F f_min = {0,};
	T_F f_max = {0,};

	int i = 0;

	do {
		f_out = 0.0;

		printf("\nF_IN : %f MHz\n", (float)f_in[i]/1000000.);
		printf("P[5:0] | M[9:0] |  F_ref  | S[2:0]|   F_out (MHz)    \n");
		printf("------------------------------------------------------------------\n");

		for(p=MIN_P; p<=MAX_P; p++) {
			for(m=MIN_M; m<=MAX_M; m+=8) {
				f_ref = (double)f_in[i]/(double)p;

				if(f_ref < MIN_F_REF) continue;
				else if(f_ref > MAX_F_REF) continue;

				state = OUTOF_F_REF;
				f_min.f_out = MAX_F_OUT;
				f_max.f_out = 0.;

				//for(s=MIN_S; s<=MAX_S; s++) {
				for(s=MAX_S; s>=MIN_S; s--) {

					f_out = (double)m * f_ref / (double)(1<<s);

					if(f_out < f_min.f_out) {
						f_min.s = s;
						f_min.p = p;
						f_min.m = m;
						f_min.f_ref = f_ref;
						f_min.f_out = f_out;
					}
					if((f_out > f_max.f_out) && (f_out <= MAX_F_OUT)) {
						f_max.s = s;
						f_max.p = p;
						f_max.m = m;
						f_max.f_ref = f_ref;
						f_max.f_out = f_out;
					}

					if(s == MAX_S) {
						printout(&f_min, &f_max);
					}

					if(f_out <= MAX_F_OUT)
						printout_fout(f_out);


					state = VALID_F_OUT;
				}

				printf("\n");
			}
		}
	} while(++i < 5);

	printf("-------------------------------------------------------------------\n\n");

	return 0;
} 

void printout_fout(double f_out)
{
	printf("%5.1f, ", f_out/1000000.);
}
void printout(T_F* min, T_F*max)
{
	printf("%6d | ", min->p);
	printf("%6d | ", min->m);
	printf("%5.2f M | ", min->f_ref/1000000.);
	printf("%1d ~ %1d | ", MIN_S, MAX_S);
//	printf("%7.1f ~ %-7.1f | \n", min->f_out/1000000., max->f_out/1000000.);
//	printf("%4.2f M \n", (max->f_out - min->f_out)/(double)(((1<<min->s))-(1<<max->s))/1000000.);
}
