#!/bin/bash
mpirun -np 4 ./a.out $1
for ((c=0; c<30; c++))
do
    result=$((time mpirun -np 4 ./a.out $1) 2>&1)
    echo $result >> $2
done
