#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "mpi.h"

void Merge(int *A, int low, int middle, int high)
{
	int *lhs, *rhs;
	int l_count = middle-low+1;
	int r_count = high-middle;
	lhs = (int*)malloc(sizeof(int) * l_count);
	rhs = (int*)malloc(sizeof(int) * r_count);
	
	int i, j, k;
	for(i=0; i<l_count; i++)
		lhs[i] = A[low+i];
	for(j=0; j<r_count; j++)
		rhs[j] = A[middle+1+j];
	i=0; j=0;
	for(k=0; k<=high && i<l_count && j<r_count; k++)
	{
		if(lhs[i]<rhs[j])
		{
			A[low+k] = lhs[i];
			i++;
		}
		else
		{
			A[low+k] = rhs[j];
			j++;
		}
	}
	if(j == r_count)
		for( ; k<=high && i<l_count; k++, i++)
			A[low+k] = lhs[i];
	else if(i == l_count)
		for( ; k<=high && j<r_count; k++, j++)
			A[low+k] = rhs[j];
	return;
}

void Merge_Sort(int *A, int low, int high)
{
	if(low < high)
	{
		int middle = (low+high) / 2;
		Merge_Sort(A, low, middle);
		Merge_Sort(A, middle+1, high);
		Merge(A, low, middle, high);
	}
	return ;
}

void main(int argc, char* argv[])
{
	int A[27]={15,46,48,93,39,6,72,91,14,36,69,40,89,61,97,12,21,54,53,97,84,58,32,27,33,72,20};
	int n=27;
	int num_procs, my_id, source;
	
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
	//均匀划分&&局部排序
	int step = n/num_procs;
	int low = my_id*step;
	int high = (my_id+1)*step-1;
	Merge_Sort(A, low, high);
	//选取样本:每个进程选取num_procs个样本
	int *Sample_one = (int*)malloc(sizeof(int)*num_procs);
	int index = 0;
	for(int i=0; i<step; i+=num_procs)
	{
		Sample_one[index] = A[my_id*step+i];
		index++;
	}
	//样本排序&&选取主元:0号进程收集其他进程所有的样本数据并排序，之后选出主元并由0号进程广播给其他进程
	int *pivot = (int*)malloc(sizeof(int)*(num_procs-1));
	int *Sample = (int*)malloc(sizeof(int)*(num_procs*num_procs));
	MPI_Gather(Sample_one, num_procs, MPI_INT, Sample, num_procs, MPI_INT, 0, MPI_COMM_WORLD);
	if(my_id == 0)
	{
		Merge_Sort(Sample, 0, num_procs*num_procs-1);
		for(int i=0; i<num_procs-1; i++)
			pivot[i] = Sample[(i+1)*num_procs];
	}
	MPI_Bcast(pivot, num_procs-1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
	//主元划分:每个进程按照主元对片段划分，结果存入buffer中，用length记录长度
	int **buffer, *length;
	buffer = (int**)malloc(sizeof(int*)*num_procs);
	length = (int*)malloc(sizeof(int)*num_procs);
	for(int i=0; i<num_procs; i++)
	{
		buffer[i] = (int*)malloc(sizeof(int)*(step+1));
		length[i] = 0;
	}
	for(int i=low; i<=high; i++)
	{
		for(int j=0; j<num_procs; j++)
		{
			if(A[i]<=pivot[j])
			{
				buffer[j][length[j]++] = A[i];
				break;
			}
			else if(j == num_procs-1)
				buffer[j][length[j]++] = A[i];
		}
	}
	MPI_Barrier(MPI_COMM_WORLD);
	//全局交换
	//首先进行长度信息的交换
	int *new_length = (int*)malloc(sizeof(int)*num_procs);
	MPI_Alltoall(length, 1, MPI_INT, new_length, 1, MPI_INT, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD); 
	int l = 0;
	for(int i=0; i<num_procs; i++)
		l += new_length[i];
	//分配相应长度的空间，并开始全局交换
	int *new_buf = (int*)malloc(sizeof(int)*l);
	int addr = 0;
	for(int i=0; i<num_procs; i++)
		if(i != my_id)
			MPI_Send(buffer[i], length[i], MPI_INT, i, i, MPI_COMM_WORLD);
	for(source=0; source<num_procs; source++)
		if(source != my_id)
		{
			MPI_Recv(new_buf+addr, new_length[source], MPI_INT, source, my_id, MPI_COMM_WORLD, &status);
			addr += new_length[source];
		}
		else
		{
			for(int i=0; i<new_length[source]; i++)
				new_buf[addr+i] = buffer[source][i];
			addr += new_length[source];
		}
	MPI_Barrier(MPI_COMM_WORLD); 
	//归并排序
	Merge_Sort(new_buf, 0, l-1);
	printf("process %d :: ", my_id);
	for(int i=0; i<l; i++)
		printf("%d ", new_buf[i]);
	printf("\n\n");
	//结果写回统一数组:先交换长度信息，再交换数据
	int *l_all = (int*)malloc(sizeof(int)*num_procs);
	int l_lan = 0;
	MPI_Allgather(&l, 1, MPI_INT, l_all, 1, MPI_INT, MPI_COMM_WORLD);
	for(int i=0; i<my_id; i++)
		l_lan += l_all[i];
	if(my_id != 0)
		MPI_Send(new_buf, l, MPI_INT, 0, my_id, MPI_COMM_WORLD);
	else
	{
		addr = 0;
		for(source=0; source<num_procs; source++)
			if(source != my_id)
			{
				MPI_Recv(A+addr, l_all[source], MPI_INT, source, source, MPI_COMM_WORLD, &status);
				addr += l_all[source];
			}
			else
			{
				for(int i=0; i<l_all[source]; i++)
					A[addr+i] = new_buf[i];
				addr += l_all[source];
			}
		//输出A数组
		for(int i=0; i<n; i++)
			printf("%d ", A[i]);
		printf("\n");
	}
	
	MPI_Finalize();
	
	return;
}
	
				
		
		
	
	
	
	
