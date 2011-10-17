#!/bin/bash

REMOTE=0

if [ "${REMOTE}" == "1" ]
then
        HOST="rambo"
        PRHOME=/home/cschwering 
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
                ssh rambo "$CMD" || exit
        else
                $CMD || exit
        fi
}

function pr_exec_log
{
        CMD="$@"
        if [ "${REMOTE}" == "1" ]
        then
                ssh rambo "$CMD > \"${PRHOME}/out\"" || exit
        else
                $CMD > "${PRHOME}/out" || exit
        fi
}

echo "TORCS + PlanRecog @ ${HOST}" &&\
pr_exec rm -f ${PRHOME}/sub && touch ${PRHOME}/sub && tail -f ${PRHOME}/sub |\
../../../bin/torcs |\
pr_exec_log ${PRGOLOG}/ctrl -d ${PRGOLOG} -l -p ${CPUS} -t 3 -v -s -f ${PRHOME}/sub

