#!/bin/bash
TORCS="../../../dist"
BIN="${TORCS}/bin"
LIB="${TORCS}/lib"
SHARE="${TORCS}/share"
make -C ../../../ &&\
cp chs.so ${LIB}/torcs/drivers/chs/chs.so &&\
cp chs.xml ${SHARE}/games/torcs/drivers/chs/chs.xml &&\
cp ../human2/human2.so ${LIB}/torcs/drivers/human2/human2.so &&\
true #${BIN}/torcs
