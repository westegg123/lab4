#!/bin/bash

rm diff.txt
echo "" > dif.txt
echo "Begin all diffs:" >> diff.txt
for file in `ls inputs/*.x`
do
	echo "Testing: $file" >> diff.txt
	sh ./test_singular.sh "$file"
done

for file in `ls inputslab2/*.x`
do
    echo "Testing: $file" >> diff.txt
	sh ./test_singular.sh "$file"
done

for file in `ls inputslab3/*.x`
do
    echo "Testing: $file" >> diff.txt
	sh ./test_singular.sh "$file"
done


echo "Finish diffs" >> diff.txt

exit