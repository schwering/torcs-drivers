dir=${HOME}/Desktop/torcs_captures

for f in $(ls ${dir}/*.pnm)
do
        g=$(echo $f | sed -e 's/\.pnm/\.png/g')
        echo "Converting $f -> $g."
        if [ ! -f "$g" ]
        then
                pnmtopng $f >$g || exit
        fi
done

echo "Creating video."
mencoder mf://${dir}/*.png -mf w=800:h=600:fps=12.5:type=png -ovc lavc -lavcopts vcodec=mpeg4 -oac copy -o ${dir}/output.avi || exit

mplayer ${dir}/output.avi || exit

