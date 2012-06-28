set xlabel "Number of Cores"
set ylabel "Time (s)"
#set title "Execution Time vs Sequential Threshold"
set logscale x 4
set logscale y 10
#set key top left
set size 0.75,0.75
#set xtics ("32" 32, "16" 16, "8" 8, "4" 4 )
set terminal postscript eps enhanced color "NimbusSanL-Regu" fontfile "/usr/share/texmf-texlive/fonts/type1/urw/helvetic/uhvr8a.pfb" 14
set output "seq8.eps"

plot "seq8.dat" using 1:2  title "Sequential Threshold = 2"  with linespoints lw 3 ,\
"seq8.dat" using 1:3 title  "Sequential Threshold = 4" with linespoints lw 3 ,\
"seq8.dat" using 1:4 title   "Sequential Threshold = 6" with linespoints lw 3 ,\
"seq8.dat" using 1:5 title   "Sequential Threshold = 8" with linespoints lw 3

set output "seq10.eps"

plot "seq10.dat" using 1:2  title "Sequential Threshold = 4"  with linespoints lw 3 ,\
"seq10.dat" using 1:3 title  "Sequential Threshold = 6" with linespoints lw 3 ,\
"seq10.dat" using 1:4 title   "Sequential Threshold = 8" with linespoints lw 3 ,\
"seq10.dat" using 1:5 title   "Sequential Threshold = 10" with linespoints lw 3

set output "seq12.eps"

plot "seq12.dat" using 1:2  title "Sequential Threshold = 6"  with linespoints lw 3 ,\
"seq12.dat" using 1:3 title  "Sequential Threshold = 8" with linespoints lw 3 ,\
"seq12.dat" using 1:4 title   "Sequential Threshold = 10" with linespoints lw 3 ,\
"seq12.dat" using 1:5 title   "Sequential Threshold = 12" with linespoints lw 3 



