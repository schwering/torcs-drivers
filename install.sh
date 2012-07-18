#!/bin/bash
TORCS="../../../dist"
BIN="${TORCS}/bin"
LIB="${TORCS}/lib"
make -C ../../../ &&\
cp chs.so ${LIB}/torcs/drivers/chs/chs.so &&\
cp ../human/human.so ${LIB}/torcs/drivers/human/human.so &&\
#cp ../../libs/pr/libpr.so ${LIB}/torcs/lib/libpr.so &&\
true #${BIN}/torcs
