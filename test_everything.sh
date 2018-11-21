#!/bin/bash

for file in `ls inputs/*.x`
do
	echo "Testing: $file"
	./sim "$file" < run.txt > my_results.txt
	./ref_sim_lab "$file" < run.txt > ref_results.txt
	diff my_results.txt ref_results.txt
	rm my_results.txt
	rm ref_results.txt
done

for file in `ls inputs_lab2/*.x`
do
        echo "Testing: $file"
        ./sim "$file" < run.txt > my_results.txt
        ./ref_sim_lab "$file" < run.txt > ref_results.txt
        diff my_results.txt ref_results.txt
        rm my_results.txt
        rm ref_results.txt
done


echo "DONE TESTING"

exit
