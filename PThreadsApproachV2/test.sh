#!/bin/bash
./a.out $1
for ((c=0; c<30; c++))
do
    result=$((time ./a.out $1) 2>&1)
    echo $result >> $2
done
