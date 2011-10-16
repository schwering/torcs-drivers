#../../../bin/torcs | ssh rambo "./ctrl -d Documents/Prolog/ccgolog -l -p 8 -t 3 -v -s -f /home/cschwering/sub >out"

ssh rambo "rm -f sub && touch sub && tail -f sub" | ../../../bin/torcs | ssh rambo "./ctrl -d Documents/Prolog/ccgolog -l -p 8 -t 3 -v -s -f /home/cschwering/sub >out"

