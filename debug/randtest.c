#include <stdio.h>
#include <stdlib.h>
#include <time.h>

unsigned int rand_interval(unsigned int min, unsigned int max)
{
    int r;
    const unsigned int range = 1 + max - min;
    const unsigned int buckets = RAND_MAX / range;
    const unsigned int limit = buckets * range;

    /* Create equal size buckets all in a row, then fire randomly towards
     * the buckets until you land in one of them. All buckets are equally
     * likely. If you land off the end of the line of buckets, try again. */
    do
    {
        r = rand();
    } while (r >= limit);

    return min + (r / buckets);
}

int main(void)
{
	unsigned int min = 0;
	unsigned int max = 477636;

	srand(time(NULL));
	for(int i=0; i<10; i++) 
	{
		printf("[%d] RAND Number at this time : %d\n", i, rand_interval(min, max));
	}

	return 0;
}


