DIR=${HOME}/Desktop/torcs_captures
FPS=2.5

for f in $(ls ${DIR}/*.pnm)
do
        g=$(echo "$f" | sed -e 's/\.pnm/\.png/g')
        echo "Converting $f -> $g."
        if [ ! -f "$g" ]
        then
                pnmtopng "$f" >"$g" || exit
        fi
done

echo "Creating video."

if [ -f "${DIR}/sub" ];
then
        SUB="-sub ${DIR}/sub"
else
        SUB=""
fi

mencoder mf://${DIR}/*.png -mf w=800:h=600:fps=${FPS}:type=png -ovc lavc -lavcopts vcodec=mpeg4 -oac copy ${SUB} -o "${DIR}/output.avi" || exit

mplayer "${DIR}/output.avi" || exit

