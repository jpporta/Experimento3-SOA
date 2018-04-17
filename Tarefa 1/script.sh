#!/bin/bash
mkdir RES

gcc Experimento3.c -o t1 -D PROTECT

for x in [1..5]
do
	sudo ./t1 >> "RES/resProtected.txt"
done

gcc Experimento3.c -o t1

for x in [1..5]
do
	sudo ./t1 >> "RES/resNOTprotected.txt"
done
rm t1
