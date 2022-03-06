/**
 * Code developed for curricular unit Sistemas Paralelos e Distribuidos of UALG
 * By Ruben Cruz nº 64591
*/

#include <stdlib.h>
#include <omp.h>
#include <unistd.h>

//number of chunks a file will be divided
//#define FILE_CHUNK_DIVISIONS 4000
#define NUM_OF_LINES_PER_CHUNK 5

omp_lock_t chunk_read_lock, chunk_write_lock;
int n_threads_reading_chunk = 0;
int n_total_threads_reading_chunk = 0;

omp_lock_t* wait_lock;

void start_chunk_read()
{
	omp_set_lock(&chunk_read_lock);
	n_threads_reading_chunk++;
	n_total_threads_reading_chunk++;
	//if (n_threads_reading_chunk == 1)
	//{
	//	omp_set_lock(&chunk_write_lock);
	//}
	omp_unset_lock(&chunk_read_lock);
}

void end_chunk_read()
{
	omp_set_lock(&chunk_read_lock);
	n_threads_reading_chunk--;
	if (n_threads_reading_chunk == 0 && n_total_threads_reading_chunk >= omp_get_max_threads() - 1)
	{
		omp_unset_lock(&chunk_write_lock);
	}
	omp_unset_lock(&chunk_read_lock);
}

void init_locks(int nThreads)
{
	omp_init_lock(&chunk_read_lock);
	omp_init_lock(&chunk_write_lock);

	wait_lock = (omp_lock_t*)malloc((nThreads - 1) * sizeof(omp_lock_t));
	for (size_t i = 0; i < nThreads - 1; i++)
	{
		omp_init_lock(&wait_lock[i]);
		omp_set_lock(&wait_lock[i]);
	}
	
}

void start_chunk_write(int nThreads)
{
	omp_set_lock(&chunk_write_lock);
}

void end_chunk_write()
{
	n_total_threads_reading_chunk = 0;
	//omp_unset_lock(&chunk_write_lock);
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

int can_update_chunk(int *read_status, int nThreads)
{
	for (int i = 0; i < nThreads; i++)
	{
		if (read_status[i] != 0)
		{
			return 0;
		}
	}
	return 1;
}

/**
 * returns 0 if its perfect magic square
 * returns 1 if its imperfect magic square
 * returns 2 if its not a magic square
 */
int get_denomination_from_file(char *filename, unsigned long long matrix_size)
{
	int nThreads = omp_get_max_threads();
	if (nThreads < 4)
	{
		printf("program must have a minimum of 4 threads\n");
		printf("type in console: export OMP_NUM_THREADS=4\n");
		return -1;
	}

	//struct timeval start_time;
    //gettimeofday(&start_time, NULL);

	n_total_threads_reading_chunk = nThreads - 1;
	int nDiagThreads = 1;
	int nReadThread = 1;
	unsigned long long chunk_lines = matrix_size <= NUM_OF_LINES_PER_CHUNK ? matrix_size : NUM_OF_LINES_PER_CHUNK;
	int nLineThreads = (nThreads - nDiagThreads - nReadThread) / 2;
	nLineThreads = chunk_lines <= nLineThreads ? chunk_lines : nLineThreads;
	int nColumnThreads = nThreads - nDiagThreads - nReadThread - nLineThreads;
	nColumnThreads = matrix_size <= nColumnThreads ? matrix_size : nColumnThreads;
	int *read_status = (int *)calloc(nThreads - 1, sizeof(int));
	int result = 0;
	int read_lines = 0;
	//unsigned long long chunk_lines = matrix_size / FILE_CHUNK_DIVISIONS;
	//unsigned long long last_chunk_lines = chunk_lines + matrix_size % FILE_CHUNK_DIVISIONS;
	unsigned long long last_chunk_lines = chunk_lines + matrix_size % chunk_lines;
	unsigned long long current_chunk_lines;
	unsigned long long *chunk = (unsigned long long *)malloc(last_chunk_lines * matrix_size * sizeof(unsigned long long));
	int result_cpy = 0;
	unsigned long long magic_constant = get_magic_constant(matrix_size, 1, 1);
	init_locks(nThreads);

	//struct timeval end_time;
    //gettimeofday(&end_time, NULL);
	//long elapsed_time_micro_s = (end_time.tv_sec*1000000 + end_time.tv_usec) - (start_time.tv_sec*1000000 + start_time.tv_usec);
	//printf("preparations time: %ld micro_s\n", elapsed_time_micro_s); 

	//#pragma omp parallel shared(nThreads, nDiagThreads, nReadThread, nLineThreads, nColumnThreads, read_status, result, chunk, filename, matrix_size, chunk_lines, last_chunk_lines, magic_constant) private(read_lines, result_cpy, current_chunk_lines, start_time, end_time, elapsed_time_micro_s)
	#pragma omp parallel shared(nThreads, nDiagThreads, nReadThread, nLineThreads, nColumnThreads, read_status, result, chunk, filename, matrix_size, chunk_lines, last_chunk_lines, magic_constant) private(read_lines, result_cpy, current_chunk_lines)
	{
		int tID = omp_get_thread_num();
		if (tID == 0)
		{
			FILE* file;
			file = fopen(filename, "r");
			unsigned long long* temp = (unsigned long long*)malloc(last_chunk_lines * matrix_size * sizeof(unsigned long long));
			//int* read_status_copy = (int*)calloc(nThreads, sizeof(int));
			while (read_lines < matrix_size && result_cpy != 2)
			{
				//printf("Master started a cycle\n");
				//gettimeofday(&start_time, NULL);
				
				//last chunk
				if (read_lines + last_chunk_lines == matrix_size)
				{
					current_chunk_lines = last_chunk_lines;
				}
				else
				{
					current_chunk_lines = chunk_lines;
				}
				
				read_chunk(temp, file, current_chunk_lines * matrix_size);

				//while(!can_update_chunk(read_status, nThreads));

				start_chunk_write(nThreads);
				//printf("Master started updating global chunk\n");

				memcpy(chunk, temp, current_chunk_lines * matrix_size * sizeof(*chunk));
				//for (int i = 0; i < last_chunk_lines * matrix_size; i++)
				//{
				//	printf("read-chunk --- %llu\n", chunk[i]);
				//}
				read_lines+=current_chunk_lines;
				end_chunk_write();
					
				for (int j = 0; j < nThreads - 1; j++)
				{
					//read_status[j] = 1;
					omp_unset_lock(&wait_lock[j]);
				}
				
				#pragma omp critical (define_result)
				{
					result_cpy = result;
				}

				//gettimeofday(&end_time, NULL);
				//elapsed_time_micro_s = (end_time.tv_sec*1000000 + end_time.tv_usec) - (start_time.tv_sec*1000000 + start_time.tv_usec);
				//printf("Master ended a cycle in %ld micro_s\n", elapsed_time_micro_s);
		
			}
		
		
		}

		else if (tID != 0)
		{
			unsigned long long* chunk_cpy = (unsigned long long*)malloc(last_chunk_lines * matrix_size * sizeof(unsigned long long));
			unsigned long long diag_sums[2] = {0, 0};
			unsigned long long* column_sums = (unsigned long long*)malloc((matrix_size / nColumnThreads + matrix_size % nColumnThreads) * sizeof(unsigned long long));
			
			while (read_lines < matrix_size && result_cpy != 2)
			{
				//printf("Thread nº %d is started a cycle\n", tID);
				//gettimeofday(&start_time, NULL);
				//while (read_status[tID - 1] != 1);
				omp_set_lock(&wait_lock[tID-1]);
				//printf("Thread nº %d was unlocked\n", tID);

				#pragma omp critical (define_result)
				{
					if (result == 2)
					{
						exit;
					}
				}

				if (read_lines + last_chunk_lines == matrix_size)
				{
					current_chunk_lines = last_chunk_lines;
				}
				else
				{
					current_chunk_lines = chunk_lines;
				}
				
				start_chunk_read();
				memcpy(chunk_cpy, chunk, current_chunk_lines * matrix_size * sizeof(*chunk_cpy));
				//read_status[tID - 1] = 0;
				end_chunk_read();
				//for (unsigned long long i = 0; i < current_chunk_lines * matrix_size; i++)
				//{
				//	printf("thread: %d --- %llu\n", tID, chunk_cpy[i]);
				//
				//}
				
				//diag threads
				if (tID <= nDiagThreads)
				{
					//printf("Thread nº %d is a diagThread\n", tID);
					for (unsigned long long i = 0; i < current_chunk_lines; i++)
					{
						diag_sums[0] += chunk_cpy[i * matrix_size + (read_lines + i)];
						diag_sums[1] += chunk_cpy[i * matrix_size + (matrix_size - ((read_lines + i) + 1))];
					}
					if (read_lines + last_chunk_lines == matrix_size)
					{
						result_cpy = diag_sums[0] != magic_constant || diag_sums[1] != magic_constant ? 1 : 0;
					}

				}

				//line threads
				else if (tID <= nDiagThreads + nLineThreads)
				{
					unsigned long long lines_to_calc = current_chunk_lines / nLineThreads;
					unsigned long long start = (tID - nDiagThreads - 1) * lines_to_calc;
					//printf("Thread nº %d is a lineThread that processes lines %llu to %llu\n", tID, start, start + lines_to_calc); 
					//last line thread must do remainder lines
					if (tID == nDiagThreads + nLineThreads - 1)
					{
						lines_to_calc += current_chunk_lines % nLineThreads;
					}
					for (unsigned long long i = start; i < start + lines_to_calc; i++)
					{
						unsigned long long line = 0;
						for (unsigned long long j = 0; j < matrix_size; j++)
						{
							line += chunk_cpy[i * matrix_size + j];
						}
						if (line != magic_constant)
						{
							result_cpy = 2;
							exit;
						}
					}
				}
				//column threads
				else if (tID <= nDiagThreads + nLineThreads + nColumnThreads)
				{
					unsigned long long cols_to_calc = matrix_size / nColumnThreads;
					unsigned long long start = (tID - nDiagThreads - nLineThreads - 1) * cols_to_calc;
					//printf("Thread nº %d is a colThread that processes cols %llu to %llu\n", tID, start, start + cols_to_calc); 
					if (tID == nDiagThreads + nLineThreads + nColumnThreads - 1)
					{
						cols_to_calc += matrix_size % nColumnThreads;
					}
					for (unsigned long long j = start; j < start + cols_to_calc; j++)
					{
						for (unsigned long long i = 0; i < current_chunk_lines; i++)
						{
							column_sums[j - start] += chunk_cpy[i * matrix_size + j];
						}
					}
					if (read_lines + last_chunk_lines == matrix_size)
					{
						for (unsigned long long i = 0; i < cols_to_calc; i++)
						{
							if (column_sums[i] != magic_constant)
							{
								result_cpy = 2;
								exit;
							}
						}
					}
					
				}
				else
				{
					exit;
				}

				read_lines += current_chunk_lines;

				#pragma omp critical (define_result)
				{
					if (result != result_cpy)
					{
						result = result >= result_cpy ? result : result_cpy;
						result_cpy = result;
					}
				}

				//gettimeofday(&end_time, NULL);
				//elapsed_time_micro_s = (end_time.tv_sec*1000000 + end_time.tv_usec) - (start_time.tv_sec*1000000 + start_time.tv_usec);
				//printf("Thread nº %d ended a cycle in %ld micro_s\n", tID,elapsed_time_micro_s);
			}
		
		}
	}

	return result;
}