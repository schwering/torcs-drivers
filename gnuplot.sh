#!/bin/sh

gnuplot -e 'plot "/tmp/track_points" with lines using 1:2' -p

