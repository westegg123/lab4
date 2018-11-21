#!/bin/bash

cycles=$1
echo "run 1" > run.txt
echo "rdump" >> run.txt

for ((i = 1; i <= (cycles-1); i++))
do
        echo "run 1" >> run.txt
        echo "rdump" >> run.txt
done
echo "quit" >> run.txt

exit
