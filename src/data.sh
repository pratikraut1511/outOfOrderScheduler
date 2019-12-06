#!/bin/bash
clear
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