#!/bin/bash
clear
make clean
rm -rf report
make all
mkdir report
echo Started

for TRACE in val_perl_trace_mem val_gcc_trace_mem
do
	mkdir ./report/${TRACE}
	INPUT=./traces/${TRACE}.txt
	for S in 8 16 32 64 128 256
	do
		for N in 1 2 4 8
		do
			FILE="./report/${TRACE}/output_${S}_${N}.txt"
			./sim $S $N 0 0 0 0 0 $INPUT > $FILE
		done
	done
done

for subFolder in ./report/*
do
	echo subFolder = $subFolder
	cd $subFolder
	rm -rf data.txt
	for file in *
	do
		echo -n $file >> data.txt
		echo -n " " >> data.txt
		awk '/ IPC                    = /{print $NF >> "./data.txt"}' $file
	done
	cd ../..
done

echo check output