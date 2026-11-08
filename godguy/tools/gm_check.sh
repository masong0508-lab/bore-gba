#!/bin/bash
# renders every GodGuy track on the PC and prints one line each (loudest peak, clipping, left/right balance, stereo width); the mean of the last two columns ends the report
cd "$(dirname "$0")/.." && gcc -O2 -DPCTEST -o pc_music pc_music.c -lm 2>/dev/null || exit 1
D=${1:-/tmp/gm_check}; mkdir -p $D; n=$(./pc_music list | wc -l); : > $D/report.txt
for i in $(seq 0 $((n-1))); do ./pc_music $i $D/t$i.wav ${2:-200} >> $D/report.txt; done
awk '{for(i=1;i<=NF;i++){if($i=="balance")b=$(i+1);if($i=="side/mid")w=$(i+1);if($i=="peak")p=$(i+1);if($i=="clip")c=$(i+1)} B+=(b<0?-b:b); W+=w; if(p>P)P=p; C+=c; N++} END{printf "%d tracks: max peak %.2f, clipped %d, mean |L/R balance| %.3f, mean side/mid %.2f\n",N,P,C,B/N,W/N}' $D/report.txt
