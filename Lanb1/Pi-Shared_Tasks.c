#include <stdio.h>
#include <omp.h>
#include <time.h>

static long num_steps = 100000;
double step;
#define NUM_THREADS 2

void main()
{
	clock_t start = clock();
	int i;
	double x, pi, sum[NUM_THREADS];
	step = 1.0/(double)num_steps;
	#pragma omp parallel // parallel regions start
	{
		double x;
		int id;
		id = omp_get_thread_num();
		sum[id] = 0;
		#pragma omp for // not given chunk,average and contiguous allocation
		for (i=0; i<num_steps; i++)
		{
			x = (i+0.5)*step;
			sum[id] += 4.0/(1.0+x*x);
		}
	} // thread 0 is 0-49999, thread 1 is 50000-99999
	for(i=0, pi=0.0; i<NUM_THREADS; i++) pi += sum[i]*step;
	clock_t finish = clock();
	double runingtime = (finish-start)/(clock_t)1000;
	printf("%f\n", pi);
	
}
