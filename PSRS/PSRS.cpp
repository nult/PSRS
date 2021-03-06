#include "stdafx.h"
#include "stdio.h"
#include "stdlib.h"
#include "mpi.h"
#include "time.h"
#define MPICH_SKIP_MPICXX 1
#define N 64
int cmpfunc(const void * a, const void * b);
int multimerge(int *data, int *ind, int n, int *data1);
int merge(int *data1, int s1, int s2, int *data2);
int main(int argc, char** argv)
{
	int rank, size;
	long nper;
	int i, j, k, accumloc1, accumloc2;
	int *value, *temp, *pivot1, *pivot2, *cate, *loc, *value2, *loc2, *steplong, *temploc2, *value3, *loc1, *arraysize;
	double start, end;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	//MPI_Status status;
	nper = N / size;
	value = (int *)malloc(N * sizeof(int));
	value2 = (int *)malloc(N * sizeof(int));
	value3 = (int *)malloc(N * sizeof(int));
	temp = (int *)malloc(size * sizeof(int));
	pivot1 = (int *)malloc(size*size * sizeof(int));
	pivot2 = (int *)malloc((size - 1) * sizeof(int));
	cate = (int *)malloc((size - 1) * sizeof(int));
	loc = (int *)malloc((size + 1) * sizeof(int));
	steplong = (int *)malloc(size*size * sizeof(int));
	temploc2 = (int *)malloc(size*size * sizeof(int));
	loc1 = (int *)malloc(size*size * sizeof(int));
	loc2 = (int *)malloc(size*size * sizeof(int));
	arraysize = (int *)malloc(size * sizeof(int));
	for (i = 0; i < size - 1; i++) cate[i] = 0;
	for (i = 0; i < size + 1; i++) loc[i] = 0;
	for (i = 0; i < size*size; i++) loc2[i] = 0;
	srand(rank);
	if (rank == 0)start = clock();
	printf("\nproc %d, before qsort", rank);
	for (i = 0; i<nper; i++)
	{
		value[i] = (int)rand();
		printf("%d ", value[i]);
	}
	printf("\nproc %d, after qsort",rank);
	MPI_Barrier(MPI_COMM_WORLD);
	qsort(value, nper, sizeof(int), cmpfunc);
	for (i = 0; i<nper; i++) printf("%d ", value[i]);
	for (i = 0; i < size; i++) {
		temp[i] = value[(i + 1)*nper / size - 1];
		//printf("%d ", temp[i]);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	//处理器0收集主元并排序,选取size-1个主元
	MPI_Gather(temp, size, MPI_INT, pivot1, size, MPI_INT, 0, MPI_COMM_WORLD);
	if (rank == 0) {
		printf("\nproc %d,before sort:",rank);
		for (i = 0; i < size*size; i++) printf("%d ", pivot1[i]);
		qsort(pivot1, size*size, sizeof(int), cmpfunc);
		printf("\nproc %d,after sort:",rank);
		for (i = 0; i < size*size; i++) printf("%d ", pivot1[i]);
		printf("\nproc %d,choose pivot2:",rank);
		for (i = 0; i < size - 1; i++) {
			pivot2[i] = pivot1[(i + 1)*size];
			printf("%d ",pivot2[i]);
		}
	}
	//if (rank == 1) {
	//printf("\nbefore bcast:");
	//for (i = 0; i < size - 1; i++)
	//printf("%d ", pivot2[i]);
	//}
	//将size-1个主元广播出去
	MPI_Bcast(pivot2, size - 1, MPI_INT, 0, MPI_COMM_WORLD);
	//if (rank == 1) {
	//printf("\nafter bcast:");
	//for (i = 0; i < size - 1; i++)
	//printf("%d ", pivot2[i]);
	//}
	//将元素进行分类，按照与主元的相对大小
	for (i = 0; i < nper; i++)
		for (j = 0; j < size - 1; j++)
			if (value[i] > pivot2[j]) cate[j]++;
	for (j = 1; j < size; j++) loc[j] = nper - cate[j - 1];
	loc[size] = nper;
	for (i = 0; i < size; i++) steplong[i] = loc[i + 1] - loc[i];
	//for (i=0;i<size;i++) MPI_Reduce(&loc[i],&loc2[i],1,MPI_INT,MPI_SUM,0, MPI_COMM_WORLD);
	//MPI_Bcast(loc2, size, MPI_INT, 0, MPI_COMM_WORLD);
	//以下输出分开的位置和间隔
	printf("\nproc %d,the breakpoint in the array:",rank);
	for (i = 0; i < size + 1; i++) printf("%d ", loc[i]);
	printf("\nproc %d,the steplength in the array:",rank);
	for (i = 0; i < size; i++) printf("%d ", steplong[i]);
	//收集各个处理器的值放到0处理器处理
	MPI_Gather(steplong, size, MPI_INT, temploc2, size, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Gather(value, nper, MPI_INT, value2, nper, MPI_INT, 0, MPI_COMM_WORLD);
	if (rank == 0) {
		//printf("\nlength of the array block:");
		//for (i = 0; i < size*size; i++) printf("%d ", temploc2[i]);
		accumloc1 = 0;
		accumloc2 = 0;
		for (i = 0; i < size; i++) {
			for (j = 0; j < size; j++) {
				loc1[i*size + j] = accumloc1;
				loc2[i*size + j] = accumloc2;
				accumloc1 = accumloc1 + temploc2[i*size + j];
				accumloc2 = accumloc2 + temploc2[j*size + i];
			}
		}
		printf("\nlocation1 for array:");
		for (i = 0; i < size; i++) {
		for (j = 0; j < size; j++) {
		printf("%d ", loc1[i*size + j]);
		}
		}
		printf("\nrelocated array lacation:");
		for (i = 0; i < size; i++) {
		for (j = 0; j < size; j++) {
		printf("%d ", loc2[i*size + j]);
		}
		}
		printf("\narray before exchange:");
		for (i = 0; i < N; i++) printf("%d ", value2[i]);
		for (i = 0; i < size; i++)
			for (j = 0; j < size; j++)
				for (k = 0; k < temploc2[i*size + j]; k++)
					value3[loc2[j*size + i] + k] = value2[loc1[i*size + j] + k];
		printf("\narray after exchange:");
		for (i = 0; i < N; i++)
		printf("%d ", value3[i]);
		for (i = 0; i < size*size - 1; i++)
			temploc2[i] = loc2[i + 1] - loc2[i];
		temploc2[size*size] = nper - loc2[size*size - 1];
	}
	//广播重新排序的电影和隔断点的位置
	MPI_Bcast(value3, N, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(loc2, size*size, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(temploc2, size*size, MPI_INT, 0, MPI_COMM_WORLD);
	//
	//for (i = 0; i < size; i++) {
	//if (i == rank) {
	//for (j = 0; j < loc[i + 1] - loc[i]; j++)
	//value2[j + loc2[i*size + j]] = value[loc[i]];
	//}
	//}
	//MPI_Send(value+loc[i],loc[i+1]-loc[i],MPI_INT,i,rank, MPI_COMM_WORLD);
	for (i = rank; i < size; i = i + size) {
		multimerge(&value3[loc2[i*size]], &temploc2[i*size], size, value2);
	}
	printf("\n");
	//for (i = 0; i<N; i++) printf("%d ", value3[i]);
	for (i = 0; i < size - 1; i++)arraysize[i] = loc2[(i + 1)*size] - loc2[i*size];
	arraysize[size - 1] = N - loc2[(size - 1)*size];
	//for (i = 1; i < size; i++)printf("%d ", arraysize[i]);
	//if (rank != 0)
	//MPI_Send(&value3[loc2[rank*size]], arraysize[rank], MPI_INT, 0,rank*1000, MPI_COMM_WORLD);
	//else
	//MPI_Recv(&value2[loc2[rank*size]], arraysize[rank], MPI_INT, 0,rank*1000, MPI_COMM_WORLD, &status);
	for (i = 0; i<size; i++) MPI_Bcast(&value3[loc2[i*size]], arraysize[i], MPI_INT, i, MPI_COMM_WORLD);
	if (rank == 0) {
		printf("\n");
		for (i = 0; i < N; i++)
		{
			printf("%d,", value3[i]);
		}
	}
	if (rank == 0) {
		end = clock();
		printf("\ntime used is %f", end - start);
	}
	MPI_Finalize();
	/*free(value);
	free(temp);
	free(pivot1);
	free(pivot2);
	free(value2);
	free(value3);
	free(cate);
	free(loc);
	free(steplong);
	free(temploc2);
	free(loc1);
	free(loc2);
	free(arraysize);*/
	return 0;
}

int cmpfunc(const void * a, const void * b) {
	return (*(int*)a - *(int*)b);
}

int multimerge(int *data, int *ind, int n, int *data1)
{
	int i;
	if (n == 2) {
		merge(data, ind[0], ind[1], data1);
		return 0;
	}
	merge(data, ind[0], ind[1], data1);
	ind[0] = ind[0] + ind[1];
	for (i = 1; i<n - 1; i++) {
		ind[i] = ind[i + 1];
	}
	//printf("%d %d %d \n", ind[0], ind[1], n);
	multimerge(data, ind, n - 1, data1);
}

int merge(int *data1, int s1, int s2, int *data2)
{
	int i, l, m;
	l = 0;
	m = s1;
	for (i = 0; i<s1 + s2; i++)
	{
		if (l == s1)
			data2[i] = data1[m++];
		else
			if (m == s2 + s1)
				data2[i] = data1[l++];
			else
				if (data1[l]>data1[m])
					data2[i] = data1[m++];
				else
					data2[i] = data1[l++];
	}
	for (i = 0; i<s1 + s2; i++) {
		data1[i] = data2[i];
	}
	return 0;
}