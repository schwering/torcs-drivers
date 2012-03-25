#!/bin/bash
TORCS="/home/chs/Programs/TORCS"
BIN="${TORCS}/bin"
LIB="${TORCS}/lib"
make -C ../../../ &&\
cp chs.so ${LIB}/torcs/drivers/chs/chs.so &&\
cp ../human/human.so ${LIB}/torcs/drivers/human/human.so &&\
true #${BIN}/torcs
