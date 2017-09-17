#!/bin/bash
./demogen $1 $2 $3
#./mappng $3 0 0 $1 $2 1024 1024 output.png
./mappng $3 700 440 300 300 1024 1024 output.png
eog output.png
