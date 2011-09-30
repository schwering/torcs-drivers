#!/bin/bash
ROOT="../../.."
BIN="${ROOT}/bin"
LIB="${ROOT}/lib"
make -C ${ROOT} &&\
cp chs.so ${LIB}/torcs/drivers/chs/chs.so &&\
cp ../human/human.so ${LIB}/torcs/drivers/human/human.so &&\
${BIN}/torcs
