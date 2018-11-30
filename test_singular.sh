#!/bin/bash
input=$1
echo ' ' > dumpsim_andreas
echo "$input" > my_results.txt
echo "$input" > ref_results.txt
./ref_sim_l4 "$input" < run.txt
echo "$(cat dumpsim)" > ref_results.txt
./sim "$input" < run.txt
echo "$(cat dumpsim_andreas)" > my_results.txt

echo "starting diff" >> diff.txt
diff my_results.txt ref_results.txt >> diff.txt
echo "ending diff" >> diff.txt

echo "FINITO"
