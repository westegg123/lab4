#!/bin/bash
input=$1

echo "$input" > my_results.txt
echo "$input" > ref_results.txt
./sim "$input" < run.txt >> my_results.txt
./ref_sim_l4 "$input" < run.txt >> ref_results.txt
diff my_results.txt ref_results.txt

echo "FINITO"
exit
