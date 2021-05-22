#include <stdio.h>
#include <string.h>
#include "mpi.h"

static long num_step = 100000;

void main(int argc, char* argv[])
{
	int num_procs, my_id, source;
	MPI_Status status;
	double pi = 0.0;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
	double x, sum = 0;
	double step = 1.0/(double)num_step;
	int i;
	for(i=my_id; i<num_step; i=i+num_procs) //迭代平均分配给各进程
	{
		x = (i+0.5)*step;
		sum += 4.0/(1.0+x*x);
	}
	if(my_id != 0) //非0号进程向0号进程发送计算结果
	{
		MPI_Send(&sum, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
	}
	else //0号进程计算总和
	{
		pi += sum * step;
		for(source=1; source<num_procs; source++)
		{
			MPI_Recv(&sum, 1, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &status);
			pi += sum * step;
		}
		printf("pi = %lf\n", pi);
	}
	MPI_Finalize();
	return;
}
