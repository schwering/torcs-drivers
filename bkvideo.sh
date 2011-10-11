DIR=${HOME}/Desktop/torcs_captures
FPS=$(grep ReMovieCaptureHack worldmodel.cpp | sed -e 's/^.\+(//g' | sed -e 's/);//g')

scp rambo:sub "${DIR}" &&\
scp rambo:Documents/Prolog/ccgolog/offset-*.dat "${DIR}" &&\
ssh rambo "rm Documents/Prolog/ccgolog/offset-*.dat"
CURRENT=$(pwd)

for I in $(seq 0 100000)
do
        NEWDIR="$DIR-$I"
        if [ ! -d "$NEWDIR" ]
        then
                break
        fi
done

echo mv "${DIR}" "${NEWDIR}"
mv "${DIR}" "${NEWDIR}" &&\
mkdir "${DIR}"

