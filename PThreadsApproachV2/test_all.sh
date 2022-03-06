#!/bin/bash
mkdir R$1
for file in ../MagicSquares/*
do
	short_file_name=$(echo $file | cut -d'/' -f 3)
	matrix_name=$(echo $short_file_name | cut -d'.' -f 1)

	echo > R$1/$short_file_name
	./test.sh $matrix_name R$1/$short_file_name
done
