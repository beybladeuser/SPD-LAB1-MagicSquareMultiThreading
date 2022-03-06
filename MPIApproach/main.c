/**
 * Code developed for curricular unit Sistemas Paralelos e Distribuidos of UALG
 * By Ruben Cruz nº 64591
*/

#include <stdio.h>

#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <mpi.h>

//#include "MagicSquareValidation.h"


typedef struct Matrix_file_read_data
{
    unsigned long long n;
    char* filename;

}Matrix_file_read_data;

Matrix_file_read_data make_matrix_data(char* file_prefix, char* matrix_name, char* file_sufix)
{
    Matrix_file_read_data result;

    //get matrix size
    int matrix_name_len = strlen(matrix_name) - 1;  
    char* matrix_size_temp = malloc((matrix_name_len + 1) * sizeof(char));
    char* eptr;
    strncpy(matrix_size_temp, matrix_name + 1, matrix_name_len);
	matrix_size_temp[matrix_name_len] = '\0';  
    result.n = strtoull(matrix_size_temp, &eptr, 10);

    //concat the args to form the file name
    int filename_len = strlen(file_prefix) + strlen(matrix_name) + strlen(file_sufix) + 1;
    result.filename = (char*)calloc(filename_len, sizeof(char));
    strcat(result.filename, file_prefix);
    strcat(result.filename, matrix_name);
    strcat(result.filename, file_sufix);
    strcat(result.filename, "\0");

    return result;
}

void print_matrix_data(Matrix_file_read_data data)
{
    printf("n: %llu\n", data.n);
    printf("filename: %s\n", data.filename);
}

void print_result_translated(int result)
{
    switch (result)
    {
    case 0:
        printf("This is a magic square\n");
        break;
    
    case 1:
        printf("This is a semimagic square\n");
        break;
    
    case 2:
        printf("This isnt a magic square\n");
        break;
    
    default:
        break;
    }
}

unsigned long long get_magic_constant(unsigned long long n, unsigned long long start, unsigned long long increment_ammount)
{
	unsigned long long brackets_n1 = n * n - 1;
	unsigned long long D_times_brackets_n1 = increment_ammount * brackets_n1;
	unsigned long long brackets_n2 = 2 * start + D_times_brackets_n1;
	return (n * brackets_n2) / 2;
}

void read_chunk(unsigned long long *chunk, FILE *file, unsigned long long chunk_size)
{
	unsigned long long i = 0;
	unsigned long long temp;
	while (i < chunk_size && fscanf(file, "%llu", &temp) != EOF)
	{

		chunk[i++] = temp;
	}
}

int main(int argc, char* argv[])
{

	if (argc < 2)
    {
        printf("must be in the format ./a.out matrix\n");
        printf("ex: ./a.out n500 (correspondant matrix file must be in folder MagicSquares)\n");
		MPI_Finalize();
        return 0;
    }

	MPI_Init(NULL, NULL);

	int world_size_int;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size_int);
	unsigned long long world_size = (unsigned long long)world_size_int;

	int world_rank_int;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank_int);
	unsigned long long world_rank = (unsigned long long)world_rank_int;

	//printf("rank: %d\n", world_rank);

	if (world_size < 2)
	{
		printf("This program must have at least 2 workers\n");
		MPI_Finalize();
		return 0;
	}
	
	Matrix_file_read_data data = make_matrix_data("../MagicSquares/", argv[1], ".txt");
	char *filename = data.filename;
	unsigned long long matrix_size = data.n;
	
	//printf("rank %d has world_size %d\n", world_rank, world_size);
	unsigned long long chunk_lines = matrix_size / (world_size - 1);
	unsigned long long last_chunk_lines = chunk_lines + matrix_size % (world_size - 1);
	unsigned long long current_chunk_lines;

	if (world_rank == 0)
	{
		int i = 0;

		FILE* file;
		file = fopen(filename, "r");

		//unsigned long long** send_buffers = (unsigned long long**)malloc((world_size - 1) * sizeof(unsigned long long*));
		unsigned long long* send_buffer = (unsigned long long*)malloc(matrix_size * last_chunk_lines * sizeof(unsigned long long));
		while (i < world_size - 1)
		{
			//last chunk
			if (i + 1 == world_size - 1)
			{
				current_chunk_lines = last_chunk_lines;
			}
			else
			{
				current_chunk_lines = chunk_lines;
			}
			//send_buffers[i] = (unsigned long long*)malloc(matrix_size * current_chunk_lines * sizeof(unsigned long long));
			//read_chunk(send_buffers[i], file, current_chunk_lines * matrix_size);
			read_chunk(send_buffer, file, current_chunk_lines * matrix_size);

			//for (unsigned long long j = 0; j < current_chunk_lines * matrix_size; j++)
			//{
			//	printf("rank %d will send --- %llu --- to rank %d\n", world_rank, send_buffers[i][j], i);
			//}

			MPI_Request request = MPI_REQUEST_NULL;
			//printf("started trans\n");
			//MPI_Isend(send_buffers[i], matrix_size * current_chunk_lines, MPI_UNSIGNED_LONG_LONG, i+1, 0, MPI_COMM_WORLD, &request);
			MPI_Isend(send_buffer, matrix_size * current_chunk_lines, MPI_UNSIGNED_LONG_LONG, i+1, 0, MPI_COMM_WORLD, &request);
			MPI_Wait(&request, MPI_STATUSES_IGNORE);
			i++;
			//printf("ended trans\n");

		}
		int final_result = 0;
		MPI_Request request;
		MPI_Irecv(&final_result, 1 , MPI_INT, world_size - 1, 2, MPI_COMM_WORLD, &request);
		MPI_Wait(&request, MPI_STATUSES_IGNORE);
		print_result_translated(final_result);
	}
	else
	{
		unsigned long long magic_const = get_magic_constant(matrix_size, 1, 1);
		unsigned long long true_inicial_i = (world_rank - 1) * chunk_lines;
		int result = 0;

		//last chunk
		if (world_rank == world_size - 1)
		{
			current_chunk_lines = last_chunk_lines;
		}
		else
		{
			current_chunk_lines = chunk_lines;
		}

		unsigned long long* recv_buffer = (unsigned long long*)malloc(matrix_size * current_chunk_lines * sizeof(unsigned long long));
		unsigned long long* prev_column_sums = (unsigned long long*)calloc(matrix_size, sizeof(unsigned long long));
		unsigned long long* column_sums = (unsigned long long*)calloc(matrix_size, sizeof(unsigned long long));
		unsigned long long prev_diag_sums[2] = {0,0};
		unsigned long long diag_sums[2] = {0,0};

		MPI_Request requests[3] = {MPI_REQUEST_NULL, MPI_REQUEST_NULL, MPI_REQUEST_NULL};
		MPI_Irecv(recv_buffer, matrix_size * current_chunk_lines, MPI_UNSIGNED_LONG_LONG, 0, 0, MPI_COMM_WORLD, &requests[0]);
		MPI_Wait(&requests[0], MPI_STATUSES_IGNORE);


		//printf("rank %d is waiting\n", world_rank);

		//for (unsigned long long i = 0; i < current_chunk_lines * matrix_size; i++)
		//{
		//	printf("rank %d received --- %llu\n", world_rank, recv_buffer[i]);
		//}
		
		for (unsigned long long i = true_inicial_i; i < true_inicial_i + current_chunk_lines; i++)
		{
			unsigned long long line_sum = 0;
			diag_sums[0] += recv_buffer[(i - true_inicial_i) * matrix_size + i ];
			diag_sums[1] += recv_buffer[(i - true_inicial_i) * matrix_size + (matrix_size - (i + 1))];
			for (unsigned long long j = 0; j < matrix_size; j++)
			{
				unsigned long long current_entry = recv_buffer[(i - true_inicial_i) * matrix_size + j];
				line_sum += current_entry;
				column_sums[j] += current_entry;
			}
			if (line_sum != magic_const)
			{
				result = 2;
				//printf("rank %d, noticed line %llu sums to %llu which is dif from the magic const %llu\n", world_rank, i, line_sum, magic_const);
				//printf("something I still have to figure out\n");
			}
			
		}
		
		if (world_rank != 1)
		{
			int result_temp;
			//requests[0] = MPI_REQUEST_NULL;
			MPI_Irecv(prev_column_sums, matrix_size , MPI_UNSIGNED_LONG_LONG, world_rank - 1, 1, MPI_COMM_WORLD, &requests[0]);
			MPI_Irecv(&result_temp, 1 , MPI_INT, world_rank - 1, 2, MPI_COMM_WORLD, &requests[1]);
			MPI_Irecv(prev_diag_sums, 2, MPI_UNSIGNED_LONG_LONG, world_rank - 1, 3, MPI_COMM_WORLD, &requests[2]);
			MPI_Waitall(3,requests, MPI_STATUSES_IGNORE);

			result = result_temp;

			if (result != 2)
			{
				for (unsigned long long i = 0; i < matrix_size; i++)
				{
					column_sums[i] += prev_column_sums[i];
				}
				diag_sums[0] += prev_diag_sums[0];
				diag_sums[1] += prev_diag_sums[1];
			}
		}
		
		if (world_rank != world_size - 1)
		{
			//requests[0] = MPI_REQUEST_NULL;
			//requests[1] = MPI_REQUEST_NULL;
			//requests[2] = MPI_REQUEST_NULL;
			MPI_Isend(column_sums, matrix_size , MPI_UNSIGNED_LONG_LONG, world_rank + 1, 1, MPI_COMM_WORLD, &requests[0]);
			MPI_Isend(&result, 1 , MPI_INT, world_rank + 1, 2, MPI_COMM_WORLD, &requests[1]);
			MPI_Isend(diag_sums, 2, MPI_UNSIGNED_LONG_LONG, world_rank + 1, 3, MPI_COMM_WORLD, &requests[2]);
			MPI_Waitall(3,requests, MPI_STATUSES_IGNORE);
		}
		else
		{
			if (result != 2)
			{
				if (diag_sums[0] != magic_const || diag_sums[1] != magic_const)
				{
					result = result != 2 ? 1 : 2;
				}
				
				for (unsigned long long i = 0; i < matrix_size; i++)
				{
					if (column_sums[i] != magic_const)
					{
						result = 2;
					}
					
				}
			}

			MPI_Isend(&result, 1 , MPI_INT, 0, 2, MPI_COMM_WORLD, &requests[0]);
			MPI_Wait(&requests[0], MPI_STATUSES_IGNORE);
		}
		

	}
	
	MPI_Finalize();
}