DIR=${HOME}/Desktop/torcs_captures
FPS=$(grep ReMovieCaptureHack worldmodel.cpp | sed -e 's/^.\+(//g' | sed -e 's/);//g' | grep -v "^0")

if [ "$1" != "" ]
then
        DIR="$1"
fi

echo "Creating video with ${FPS} fps"

scp rambo:sub "${DIR}" &&\
scp rambo:Documents/Prolog/ccgolog/offset-*.dat "${DIR}" &&\
ssh rambo "rm -f sub Documents/Prolog/ccgolog/offset-*.dat"
CURRENT=$(pwd)
(cd /home/chs/Documents/Prolog/ccgolog/ && ./offset.sh ${DIR}/offset-*.dat || cd "$CURRENT" && exit) && cd "$CURRENT"

for f in $(ls ${DIR}/*.pnm)
do
        g=$(echo "$f" | sed -e 's/\.pnm/\.png/g')
        echo "Converting $f -> $g."
        if [ ! -f "$g" ]
        then
                pnmtopng "$f" >"$g" || exit
                rm -f "$f"
        fi
done

echo "Creating video."

if [ -f "${DIR}/sub" ];
then
        SUB="-sub ${DIR}/sub"
else
        SUB=""
fi

mencoder mf://${DIR}/torcs-*.png -mf w=800:h=600:fps=${FPS}:type=png -ovc lavc -lavcopts vcodec=mpeg4 -oac copy ${SUB} -o "${DIR}/output.avi" || exit

mplayer -loop 0 "${DIR}/output.avi" || exit

