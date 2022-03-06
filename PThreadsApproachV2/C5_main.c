/**
 * Code developed for curricular unit Sistemas Paralelos e Distribuidos of UALG
 * By Ruben Cruz nº 64591
 * 
 * this code is the pthreads implementation with 5 threads
*/

#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include "C5_MagicSquareValidation.h"


typedef struct matrix_file_read_data
{
    unsigned long long n;
    char* filename;

}matrix_file_read_data;

matrix_file_read_data make_matrix_data(char* file_prefix, char* matrix_name, char* file_sufix)
{
    matrix_file_read_data result;

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

void print_matrix_data(matrix_file_read_data data)
{
    printf("n: %llu\n", data.n);
    printf("filename: %s\n", data.filename);
}

void check_if_magic_square_from_file(matrix_file_read_data data)
{
    
    int denomination = get_denomination_from_file(data.filename, data.n);
    switch (denomination)
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



int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("must be in the format ./a.out matrix\n");
        printf("ex: ./a.out n500 (correspondant matrix file must be in folder MagicSquares)\n");
        return 0;
    }

    check_if_magic_square_from_file(make_matrix_data("../MagicSquares/", argv[1], ".txt"));

    return 0;
}