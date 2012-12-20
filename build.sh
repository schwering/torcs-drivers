#!/bin/bash
TORCS="../../../dist"
BIN="${TORCS}/bin"
LIB="${TORCS}/lib"
SHARE="${TORCS}/share"
make -C ../../../ &&\
cp chs.so ${LIB}/torcs/drivers/chs/chs.so &&\
cp chs.xml ${SHARE}/games/torcs/drivers/chs/chs.xml &&\
cp ../human/human.so ${LIB}/torcs/drivers/human/human.so &&\
true #${BIN}/torcs
