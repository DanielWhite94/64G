#!/bin/bash
./demogen $1 $2 $3
./mappng $3 0 0 $1 $2 4096 4096 output.png
eog output.png
