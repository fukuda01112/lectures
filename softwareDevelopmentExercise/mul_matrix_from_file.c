
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>

#define MAXLEN 1024
#define OUT 0

double* readMatrix(const char* path, int* rows_num, int* cols_num);
double** readMatrix(const char* path, int* rows_num, int* cols_num);

void outputMatrix(const char* path, double* matrix,int rows_num, int cols_num);
double* mulutipyMatrix(const double* leftM, const double* rightM);
void distributeRightMat();

int main(int argc, char *argv[]) {
	int i, j, k;
	
	//matrix sizes ,Aの行&Cの行、Bの列&Cの列、Aの列&Bの行
	int I, J, K;

	char leftM_path[MAXLEN];
	char rightM_path[MAXLEN];
	char ansM_path[MAXLEN];


	if (argc == 4) {
		strcpy(leftM_path, argv[1]);
		strcpy(rightM_path, argv[2]);
		strcpy(ansM_path, argv[3]);
	} else {
		printf("ERROR USAGE: mul_matrix leftM_path leftM_path ansM_path\n");
		return 0;
	}
	
	double sum;
	double *a, *b, *partial_a;
	double *b2, *c, *partial_c;
　　
    double* leftM = readMatrix(leftM_path, &I, &K);
    double* rightM = readMatrix(rightM_path, &K, &J);

	double start,start2, end,end2;

	/* MPI Initialize */
	int rank, num_procs,alloc_a_elems_num, alloc_a_rows_num, alloc_c_elems_num;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	start = MPI_Wtime();
	
	/*rootプロセスの処理内容*/
	if (rank == 0) {
		a = readMatrix(leftM_path, &I, &K);
		int a_K = K;

		b = readMatrix(rightM_path, &K, &J);

		if(a_K!=K){
			printf("ERROR A cols_num != B rows_num");
			return 0;
		}

		alloc_c_elems_num = I * J / num_procs;
		alloc_a_elems_num= I * K / num_procs;	
		alloc_a_rows_num = I / num_procs;
	}

	MPI_Bcast(&I, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&J, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&K, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&alloc_c_elems_num, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&alloc_a_rows_num, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&alloc_a_elems_num, 1, MPI_INT, 0, MPI_COMM_WORLD);

	partial_a = (double *) malloc(alloc_a_elems_num * sizeof (double));
	partial_c = (double *) malloc(alloc_c_elems_num * sizeof (double));

	b2 = (double *) malloc(J * K * sizeof (double));

	if (rank == 0) {
		c = (double *) malloc(I * J * sizeof (double));
		b2 = (double *) malloc(J * K * sizeof (double));

		//bを転置してb2に格納
		for (k = 0; k < K; k++) {
			for (j = 0; j < J; j++) {
				b2[k * J + j] = b[j * K + k];
			}
		}

		//データを送信
		for (i = 1; i < num_procs; i++) {
			int a_first = i*alloc_a_elems_num;
			for (k = 0; k < alloc_a_elems_num; k++) {
				partial_a[k] = a[a_first + k];
			}
			MPI_Send(partial_a, alloc_c_elems_num, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
		}

		for (i = 0; i < alloc_a_elems_num; i++) {
			partial_a[i] = a[i];
		}

		free(a);
		free(b);
        printf("calc start\n");
	} else {
		MPI_Status status;
		MPI_Recv(partial_a, alloc_a_elems_num, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);
	}

	MPI_Bcast(b2, J*K, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	
	/*各プロセスの処理内容*/
	double temp;
	start2=MPI_Wtime();

    // 計算部分 tempをレジスタに置くこととアンローリングで高速化
#pragma omp parallel for private(i,k,temp)
	for (j = 0; j < J; j += 8) {
		for (i = 0; i < alloc_a_rows_num; i++) {
			for (k = 0; k < K; k++) {
				temp = partial_a[i * K + k];
				partial_c[i * J + j] += temp * b2[j * K + k];
				partial_c[i * J + j + 1] += temp * b2[(j + 1) * K + k];
				partial_c[i * J + j + 2] += temp * b2[(j + 2) * K + k];
				partial_c[i * J + j + 3] += temp * b2[(j + 3) * K + k];
				partial_c[i * J + j + 4] += temp * b2[(j + 4) * K + k];
				partial_c[i * J + j + 5] += temp * b2[(j + 5) * K + k];
				partial_c[i * J + j + 6] += temp * b2[(j + 6) * K + k];
				partial_c[i * J + j + 7] += temp * b2[(j + 7) * K + k];
			}
		}
	}

	MPI_Gather(partial_c, alloc_c_elems_num, MPI_DOUBLE, c, alloc_c_elems_num, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	/*各プロセスの処理内容終わり*/

	//rootに戻って出力
	if (rank == 0) {

	    end2 = MPI_Wtime();

		//cをファイルに出力
		if (rank == 0) {
			sum = 0;
			for (i = 0; i < I * J; i++) {
				sum += c[i];
			}
		}
		outputMatrix(ansM_path, c, I, J);
		end=MPI_Wtime();    
		free(c);

		printf("N:%d sum:%f,all_time:%f,calc_time:%f,processes:%d(alloc_c_num:%d).threads:%d\n", I*J, sum, (end - start),(end2-start2), num_procs, alloc_c_elems_num, omp_get_max_threads());
	}

	//終了処理
	free(partial_a);
	free(b2);
	free(partial_c);
	MPI_Finalize();

	return 0;
}

double* readMatrix(const char* path, int* rows_num, int* cols_num) {
	FILE* fp;
	int row, col;
	double value;
	double *matrix;

	int i, j;
	printf("read matrix %s\n", path);
	fp = fopen(path, "rb");
	if (fp != NULL) {
		fread(&row, sizeof (int), 1, fp);
		fread(&col, sizeof (int), 1, fp);
		*rows_num = row;
		*cols_num = col;

		matrix = (double *) malloc(row * col * sizeof (double));

		for (i = 0; i < row; ++i) {
			for (j = 0; j < col; ++j) {
				fread(&value, sizeof (double), 1, fp);
				printf("%d",value);
				matrix[i * col + j] = value;
			}
		}
	}
	fclose(fp);

	return matrix;
}

double** readMatrix(const char* path, int* rows_num, int* cols_num) {
	FILE* fp;
	int row, col;
	double value;
	double** matrix;

	int i, j;
	
	fp = fopen(path, "rb");
	if (fp != NULL) {
		fread(&row, sizeof (int), 1, fp);
		fread(&col, sizeof (int), 1, fp);
		// *rows_num = row;
		// *cols_num = col;
        double** matrix;
    	matrix = malloc(sizeof(int *) * col);
 

      	for(int i = 0; i < n; i++) {
		    nums1[i] = malloc(sizeof(int) * row);
	    }
	    
		for (i = 0; i < row; ++i) {
			for (j = 0; j < col; ++j) {
				fread(&value, sizeof (double), 1, fp);
				matrix[i][j] = value;
			}
		}
	}
	fclose(fp);

	return matrix;
}


void outputMatrix(const char* path, double* matrix, int rows_num, int cols_num) {
	FILE* fp;
	int i, j;
	double value;

	fp = fopen(path, "wb");
	if (fp != NULL) {

		fwrite(&rows_num, sizeof (int), 1, fp);
		fwrite(&cols_num, sizeof (int), 1, fp);
		for (i = 0; i < rows_num; ++i) {
			for (j = 0; j < cols_num; ++j) {
				value = matrix[i * cols_num + j];
				fwrite(&value, sizeof (double), 1, fp);
			}
		}
	}
	fclose(fp);
}

