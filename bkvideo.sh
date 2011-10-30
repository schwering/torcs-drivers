DIR=${HOME}/Desktop/torcs_captures

scp rambo:out "${DIR}"
scp rambo:sub "${DIR}"
scp rambo:Documents/Prolog/ccgolog/trace-*.dat "${DIR}"
ssh rambo "rm -f out sub Documents/Prolog/ccgolog/trace-*.dat"
CURRENT=$(pwd)

for I in $(seq 0 100000)
do
        NEWDIR="$DIR-$I"
        if [ ! -d "$NEWDIR" ]
        then
                break
        fi
done

(cd /home/chs/Documents/Prolog/ccgolog/ && ./pos.sh ${DIR}/trace-*.dat || cd "$CURRENT" && exit) && cd "$CURRENT"
(cd /home/chs/Documents/Prolog/ccgolog/ && ./offset.sh ${DIR}/trace-*.dat || cd "$CURRENT" && exit) && cd "$CURRENT"
echo mv "${DIR}" "${NEWDIR}"

mv "${DIR}" "${NEWDIR}" &&\
mkdir "${DIR}"

