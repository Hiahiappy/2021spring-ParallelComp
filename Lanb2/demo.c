#include <stdio.h>
#include <string.h>
#include "mpi.h"
void main(int argc, char* argv[])
{
	int num_procs, my_id, source;
	MPI_Status status;
	char message[100];
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
	if(my_id != 0)
	{
		strcpy(message, "Hello World!");
		MPI_Send(message, strlen(message)+1, MPI_CHAR, 0, 99, MPI_COMM_WORLD);
	}
	else
	{
		for(source=1; source<num_procs; source++)
		{
			MPI_Recv(message, 100, MPI_CHAR, source, 99, MPI_COMM_WORLD, &status);
			printf("接受到第 %d 号进程发送的消息：%s\n", source, message);
		}
	}
	MPI_Finalize();
}
