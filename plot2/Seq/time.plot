set xlabel "Sequential Threshold"
set ylabel "Time (s)"
#set title "Execution Time vs Sequential Threshold"
#set logscale x 4
#set logscale y 10
#set key top left
set size 0.75,0.75
#set xtics ("32" 32, "16" 16, "8" 8, "4" 4 )
set terminal postscript eps enhanced color "NimbusSanL-Regu" fontfile "/usr/share/texmf-texlive/fonts/type1/urw/helvetic/uhvr8a.pfb" 14
set output "dep8.eps"

plot "p64d8.dat" using 3:1  title "P = 64"  with linespoints lw 3 ,\
"p256d8.dat" using 4:2 title  "P = 256" with linespoints lw 3 ,\
"p1024d8.dat" using 4:2 title   "P = 1024" with linespoints lw 3

set output "dep10.eps"

plot "p64d10.dat" using 4:2  title "P = 64"  with linespoints lw 3 ,\
"p256d10.dat" using 4:2 title  "P = 256" with linespoints lw 3 ,\
"p1024d10.dat" using 4:2 title   "P = 1024" with linespoints lw 3

set output "dep12.eps"

plot "p64d12.dat" using 4:2  title "P = 64"  with linespoints lw 3 ,\
"p256d12.dat" using 4:2 title  "P = 256" with linespoints lw 3 ,\
"p1024d12.dat" using 4:2 title   "P = 1024" with linespoints lw 3



