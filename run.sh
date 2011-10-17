#!/bin/bash

REMOTE=1

if [ "$1" == "-l" -o "$1" == "--local" ]
then
        REMOTE=0
elif [ "$1" == "-r" -o "$1" == "--remote" ]
then
        REMOTE=1
fi

if [ "${REMOTE}" == "1" ]
then
        HOST="rambo"
        PRHOME=$(ssh rambo 'echo ${HOME}')
        CPUS=8
else
        HOST="$(hostname)"
        PRHOME=${HOME}
        CPUS=1
fi
PRGOLOG=${PRHOME}/Documents/Prolog/ccgolog

function pr_exec
{
        CMD="$@"
        if [ "${REMOTE}" == "1" ]
        then
                ssh ${HOST} "$CMD" || exit
        else
                eval "$CMD" || exit
        fi
}

echo "TORCS + PlanRecog" &&\
echo "HOST   = ${HOST}" &&\
echo "PRHOME = ${PRHOME}" &&\
echo "CPUS   = ${CPUS}" &&\
pr_exec rm -f ${PRHOME}/sub \&\& touch "${PRHOME}/sub" \&\& tail -f "${PRHOME}/sub" |\
../../../bin/torcs |\
pr_exec "${PRGOLOG}/ctrl" -d "${PRGOLOG}" -l -p ${CPUS} -t 3 -v -s -f "${PRHOME}/sub" \> "${PRHOME}/out"

