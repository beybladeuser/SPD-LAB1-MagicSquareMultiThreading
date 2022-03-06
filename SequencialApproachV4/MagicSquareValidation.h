/**
 * Code developed for curricular unit Sistemas Paralelos e Distribuidos of UALG
 * By Ruben Cruz nº 64591
*/

//#include <stdio.h>
#include <stdlib.h>


unsigned long long get_magic_constant(unsigned long long n, unsigned long long start, unsigned long long increment_ammount)
{
    unsigned long long brackets_n1 = n*n - 1;
    unsigned long long D_times_brackets_n1 = increment_ammount * brackets_n1;
    unsigned long long brackets_n2 = 2*start + D_times_brackets_n1;
    return (n * brackets_n2) / 2;
}


/**
 * returns 0 if its perfect magic square
 * returns 1 if its imperfect magic square
 * returns 2 if its not a magic square
 */
int get_denomination_from_file(char* filename, unsigned long long matrix_size)
{
    unsigned long long magic_constant = get_magic_constant(matrix_size, 1, 1);

    unsigned long long* collumns_sums = (unsigned long long*)calloc(matrix_size, sizeof(unsigned long long));
    unsigned long long diag_sums[] = {0, 0};
    
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

    for (unsigned long long i = 0; i < matrix_size; i++)
    {
        if (collumns_sums[i] != magic_constant)
        {
            return 2;
        }
    }

    for (int i = 0; i < 2; i++)
    {
        if (diag_sums[i] != magic_constant)
        {
            return 1;
        }
    }
    
    return 0;
}