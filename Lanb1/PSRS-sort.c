#include <stdio.h>
#include <omp.h>
#include <math.h>
#include <malloc.h>

#define p 3

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

void main()
{
	int id, step, index;
	int Sample[p*p], pivot[p-1];
	int i;
	//输入待排序的数据
	
	int A[27]={15,46,48,93,39,6,72,91,14,36,69,40,89,61,97,12,21,54,53,97,84,58,32,27,33,72,20};
	int n=27;
	//初始化一些变量
	index = 0;
	step = n/p;
	omp_set_num_threads(p);
	//均匀划分：划分为num_threads段
	#pragma omp parallel shared(index) private(id)
	{
		id = omp_get_thread_num();
	//局部排序：调用串行排序算法对每个划分进行串行排序
		int low = id * step;
		int high = (id+1)*step-1;
		Merge_Sort(A, low, high);
	//选取样本：正则采样 ？？？？
		#pragma omp critical
		{
		for(int k=0; k<step; k=k+p)
			Sample[index++] = A[id*step+k];
		}
	//采样排序
		#pragma omp barrier
		#pragma omp master
		{
			Merge_Sort(Sample, 0, p*p-1);
	//选择p-1个主元
			for(int m=0; m<p-1; m++)
				pivot[m] = Sample[(m+1)*p];
		}
	}
	//主元划分 & 全局交换
	int **Partition;
	Partition=(int**)malloc(sizeof(int*)*p);
	for(int j=0; j<p; j++)
		Partition[j]=(int*)malloc(sizeof(int)*(n+1));
	int length[p]={0,0,0};
	#pragma omp parallel shared(length,Partition) private(id)
	{
		id = omp_get_thread_num();
		int j, k;
		#pragma omp critical
		{
		for(j=0; j<step; j++)
		{
			for(k=0; k<p; k++)
			{
				if(A[id*step+j]<pivot[k])
				{
					Partition[k][length[k]++]=A[id*step+j];
					break;
				}
				else if(k==p-1)
				{
					Partition[k][length[k]++]=A[id*step+j];
				}
			}
		}
		}
		#pragma omp barrier
		Merge_Sort(Partition[id], 0, length[id]-1);
	}
	int addr=0;
	printf("Result: ");
	for(int j=0; j<p; j++)
	{
		for(i=0; i<length[j]; i++)
		{
			A[addr++]=Partition[j][i];
			printf("%d ", Partition[j][i]);
		}
	}
		printf("\n");
	return;
}
		
