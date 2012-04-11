#!/bin/bash

SCENARIO_FILE="../scenario.h"
REMOTE=1
TORCS="/home/chs/Programs/TORCS"

if [ "$1" == "-l" -o "$1" == "--local" ]
then
	REMOTE=0
elif [ "$1" == "-r" -o "$1" == "--remote" ]
then
	REMOTE=1
fi

SCENARIO="$1"
if [ "$SCENARIO" != "overtake" -a "$SCENARIO" != "critical" ]
then
	SCENARIO="$2"
fi
if [ "$SCENARIO" != "overtake" -a "$SCENARIO" != "critical" ]
then
	echo "Scenario is empty."
	SCENARIO="overtake"
	echo "Fallback to $SCENARIO. Press enter to continue."
	read
fi

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
        if [ "${REMOTE}" == "1" ]
        then
                HOST="rambo"
                vpn_connect || exit
                PRHOME=$(ssh rambo 'echo ${HOME}')
                WORKERS=24
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

cp ../human/human.xml.$SCENARIO ${TORCS}/share/games/torcs/drivers/human/human.xml || exit
cp ../../raceman/quickrace.xml.$SCENARIO ${TORCS}/share/games/torcs/config/raceman/quickrace.xml || exit

if [ "$(grep "$SCENARIO" "$SCENARIO_FILE")" == "" ]
then
	echo "// $SCENARIO" >"$SCENARIO_FILE"
	if [ "$SCENARIO" == "overtake" ]
	then
		echo "#define DA_SPEED_LIMIT" >>"$SCENARIO_FILE"
	fi
	if [ "$SCENARIO" == "critical" ]
	then
		echo "#define DA_ACCEL_LIMIT" >>"$SCENARIO_FILE"
	fi
	for CPPFILE in $(find .. -name \*.cpp -or -name \*.h | xargs grep -l "$SCENARIO_FILE")
	do
		touch $CPPFILE
	done
	./install.sh || exit
fi

echo "HOST    = ${HOST}"
echo "PRHOME  = ${PRHOME}"
echo "WORKERS = ${WORKERS}"

pr_exec rm -f ${PRHOME}/short-info \&\& tail -F "${PRHOME}/short-info" |\
${TORCS}/bin/torcs |\
pr_exec "${PRGOLOG}/ctrl" --working-dir "${PRGOLOG}" --module "cs2" --scenario "$SCENARIO" --live --heuristic --workers ${WORKERS} --interval 1 --verbose --short-info "${PRHOME}/short-info" --stdin-dump "${PRHOME}/stdin" \> "${PRHOME}/out"
