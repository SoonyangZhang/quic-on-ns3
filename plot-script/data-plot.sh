#! /bin/sh
algo=$1
id=$2
prefix1=${id}_10.1.1.1:49153_10.1.1.2:1234
prefix2=${id}_10.1.1.1:49154_10.1.1.2:1234
prefix3=${id}_10.1.1.1:49155_10.1.1.2:1234
file1=${prefix1}_sendrate.txt
file2=${prefix2}_sendrate.txt
file3=${prefix3}_sendrate.txt
output=${id}-${algo}
gnuplot<<!
set xlabel "time/s" 
set ylabel "rate/kbps"
set xrange [0:200]
set yrange [0:5000]
set term "png"
set output "${output}-send-rate.png"
plot "${file1}" u 1:2 title "flow1" with lines lw 2 lc 1,\
"${file2}" u 1:2 title "flow2" with lines lw 2 lc 2,\
"${file3}" u 1:2 title "flow3" with lines lw 2 lc 3
set output
exit
!
file1=${prefix1}_goodput.txt
file2=${prefix2}_goodput.txt
file3=${prefix3}_goodput.txt
output=${id}-${algo}
gnuplot<<!
set xlabel "time/s" 
set ylabel "rate/kbps"
set xrange [0:200]
set yrange [0:5000]
set term "png"
set output "${output}-goodput.png"
plot "${file1}" u 1:2 title "flow1" with lines lw 2 lc 1,\
"${file2}" u 1:2 title "flow2" with lines lw 2 lc 2,\
"${file3}" u 1:2 title "flow3" with lines lw 2 lc 3
set output
exit
!
file1=${prefix1}_inflight.txt
file2=${prefix2}_inflight.txt
file3=${prefix3}_inflight.txt
gnuplot<<!
set xlabel "time/s" 
set ylabel "inflight (p)"
set xrange [0:200]
set yrange [0:200]
set term "png"
set output "${output}-inflight.png"
plot "${file1}" u 1:2 title "flow1" with lines lw 2 lc 1,\
"${file2}" u 1:2 title "flow2" with lines lw 2 lc 2,\
"${file3}" u 1:2 title "flow3" with lines lw 2 lc 3
set output
exit
!

file1=${prefix1}_owd.txt
file2=${prefix2}_owd.txt
file3=${prefix3}_owd.txt
gnuplot<<!
set xlabel "time/s" 
set ylabel "delay/ms"
set xrange [0:200]
set yrange [100:400]
set term "png"
set output "${output}-owd.png"
plot "${file1}" u 1:3 title "flow1" with lines lw 2
set output
exit
!
