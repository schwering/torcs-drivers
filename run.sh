#!/bin/bash

REMOTE=1

function vpn_connect
{
        if [ "${HOST}" == "rambo" -a "$(ifconfig | grep 'inet addr' | grep '13[47]')" == "" -a "$(ifconfig | grep 'inet addr' | grep '172.16.')" == "" ]
        then
                echo "Connecting to VPN"
		if [ "$(which vpnc-connect 2>/dev/null)" != "" ]
		then
			sudo vpnc-connect || exit
		elif [ "$(which vpnc 2>/dev/null)" != "" ]
		then
			sudo vpnc || exit
		fi
        fi
}

function init_vars
{
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
                vpn_connect || exit
                PRHOME=$(ssh rambo 'echo ${HOME}')
                WORKERS=8
        else
                HOST="$(hostname)"
                PRHOME=${HOME}
                WORKERS=1
        fi

        PRGOLOG=${PRHOME}/Documents/Prolog/ccgolog
}

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

echo "TORCS + PlanRecog"

init_vars || exit

echo "HOST    = ${HOST}"
echo "PRHOME  = ${PRHOME}"
echo "WORKERS = ${WORKERS}"

pr_exec rm -f ${PRHOME}/short-info \&\& tail -F "${PRHOME}/short-info" |\
../../../bin/torcs |\
pr_exec "${PRGOLOG}/ctrl" --working-dir "${PRGOLOG}" --module "cs2" --live --heuristic --workers ${WORKERS} --interval 1 --short-info "${PRHOME}/short-info" --stdin-dump "${PRHOME}/stdin" \> "${PRHOME}/out"

