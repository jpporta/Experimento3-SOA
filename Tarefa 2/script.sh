#!/bin/bash

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
