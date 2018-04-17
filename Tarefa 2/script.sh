#!/bin/bash

gcc t2.c -o t2 -D PROTECT

for x in [1..5]
do
	sudo ./t2 >> "RES/resProtected.txt"
done

gcc t2.c -o t2

for x in [1..5]
do
	sudo ./t2 >> "RES/resNOTprotected.txt"
done
rm t2
