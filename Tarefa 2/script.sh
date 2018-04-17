#!/bin/bash

<<<<<<< HEAD
gcc t2.c -o t2 -D PROTECT

for x in [1..5]
do
	sudo ./t2 >> resProtected.txt
done

gcc t2.c -o t2

for x in [1..5]
do
	sudo ./t2 >> resNOTprotected.txt
done
rm t2

=======
if [ -d "./RES" ]; then
  rm -rf RES
fi
mkdir RES
gcc t2.c -o t2cp -D PROTECT
gcc t2.c -o t2sp
for numVezesP in {1..5}
do
  echo "Teste ${numVezesP}, com PROTECT"
  sudo ./t2cp >> "RES/RES-Protect-${numVezesP}.txt"
  sleep 2
done
for numVezesSP in {1..5}
do
  echo "Teste ${numVezesSP}, sem PROTECT"
  sudo ./t2sp >> "RES/RES-No-Protect-${numVezesSP}.txt"
  sleep 2
done
>>>>>>> 52bddb7179fbb18d34277f8f588d42d00c93662c
