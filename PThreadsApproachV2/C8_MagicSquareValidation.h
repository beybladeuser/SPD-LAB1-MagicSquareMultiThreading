/**
 * Code developed for curricular unit Sistemas Paralelos e Distribuidos of UALG
 * By Ruben Cruz nº 64591
 * 
 * this code is the pthreads implementation with 8 threads
*/

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <semaphore.h>

#define NUM_OF_THREADS 8

sem_t denomination_write_semaphore;
sem_t denomination_read_semaphore;
int n_threads_reading_semaphore = 0;
int denomination = 0;

typedef struct {
    unsigned long long* sums;
    unsigned long long start;
    unsigned long long end;
    int result_in_case_of_failure;
    unsigned long long magic_const; 
}Args;

Args args(unsigned long long* sums, unsigned long long start, unsigned long long end, int result_in_case_of_failure, unsigned long long magic_const)
{
    Args result;
    result.sums = sums;
    result.start = start;
    result.end = end;
    result.result_in_case_of_failure = result_in_case_of_failure;
    result.magic_const = magic_const;

    return result;
}

unsigned long long get_magic_constant(unsigned long long n, unsigned long long start, unsigned long long increment_ammount)
{
    unsigned long long brackets_n1 = n*n - 1;
    unsigned long long D_times_brackets_n1 = increment_ammount * brackets_n1;
    unsigned long long brackets_n2 = 2*start + D_times_brackets_n1;
    return (n * brackets_n2) / 2;
}

void start_denomination_read()
{
    sem_wait(&denomination_read_semaphore);
    n_threads_reading_semaphore++;
    if (n_threads_reading_semaphore == 1)
    {
        sem_wait(&denomination_write_semaphore);
    }
    sem_post(&denomination_read_semaphore);
}

void end_denomination_read()
{
    sem_wait(&denomination_read_semaphore);
	n_threads_reading_semaphore--;
	if(n_threads_reading_semaphore == 0)
    {
        sem_post(&denomination_write_semaphore);
    }
	sem_post(&denomination_read_semaphore);
}



void* check_array_if_equal_to_magic_const(void* args_v)
{
    Args* args=(Args*)args_v;
    

    for (unsigned long long i = args->start; i < args->end; i++)
    {      
        if (args->sums[i] != args->magic_const)
        {
            sem_wait(&denomination_write_semaphore);

            denomination = denomination == 2 ? 2 : args->result_in_case_of_failure;

            sem_post(&denomination_write_semaphore);

            pthread_exit(0);
            return NULL;
        }
    }
    pthread_exit(0);
    
}

void create_threads_for_column_diag_check(pthread_t* threads, int n_threads, unsigned long long* collumns_sums, unsigned long long* diag_sums, unsigned long long matrix_size, unsigned long long magic_const)
{
    if (n_threads <= 1)
    {
        n_threads = 2;
    }
    
    //the number of threads that check the columns
    int n_collumn_threads = n_threads - 1;

    unsigned long long load_for_col = n_collumn_threads < matrix_size ? matrix_size / n_collumn_threads : 1 ;

    for (unsigned long long i = 0; i < n_collumn_threads && i < matrix_size - 1; i++)
    {
        unsigned long long start = i * load_for_col;
        unsigned long long end = i == n_collumn_threads -1 ? matrix_size : start + load_for_col;
        Args argument = args(collumns_sums, start, end, 2, magic_const);
        int code=pthread_create(&threads[i],NULL,check_array_if_equal_to_magic_const,(void*)&argument);
		if (code){
			printf("ERROR; return code from pthread_create() is %d\n", code);
			exit(-1);
		}
    }

    Args argument = args(diag_sums, 0, 2, 1, magic_const);
    int code=pthread_create(&threads[n_threads - 1],NULL,check_array_if_equal_to_magic_const,(void*)&argument);
	if (code){
		printf("ERROR; return code from pthread_create() is %d\n", code);
		exit(-1);
	}
    for (unsigned long long i = 0; i < n_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }
    
}

/**
 * returns 0 if its perfect magic square
 * returns 1 if its imperfect magic square
 * returns 2 if its not a magic square
 */
int get_denomination_from_file(char* filename, unsigned long long matrix_size)
{
    denomination = 0;
    unsigned long long magic_constant = get_magic_constant(matrix_size, 1, 1);

    unsigned long long* collumns_sums = (unsigned long long*)calloc(matrix_size, sizeof(unsigned long long));
    unsigned long long* diag_sums = (unsigned long long*)calloc(2, sizeof(unsigned long long));
    
    FILE* file;
    file = fopen(filename, "r");
        
    unsigned long long i = 0;
    unsigned long long j = 0;
    unsigned long long temp;
    unsigned long long current_line_sum = 0;

    while (fscanf(file, "%llu", &temp) != EOF)
    {
        current_line_sum += temp;
        collumns_sums[j] += temp;

        diag_sums[0] = i == j ? diag_sums[0] + temp : diag_sums[0];

        diag_sums[1] = j == matrix_size - (i + 1) ? diag_sums[1] + temp : diag_sums[1];

        j++;
        if (j % matrix_size == 0)
        {
            if (current_line_sum != magic_constant)
            {
                return 2;
            }
            j = 0;
            i++;
            current_line_sum = 0;
        }
    }

    int min_num_threads = NUM_OF_THREADS >= matrix_size ? matrix_size : NUM_OF_THREADS;
    pthread_t threads[min_num_threads];
    create_threads_for_column_diag_check(threads, min_num_threads, collumns_sums, diag_sums, matrix_size, magic_constant);
    
    return denomination;
}