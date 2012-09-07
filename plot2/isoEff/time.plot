set xlabel "Number of cores"
set ylabel "Parallel Efficiency (wrt 16 cores)"
#set title "Startup Time vs Number of Cores: Comparison with OpenMPI Startup"
set logscale x 4
#set logscale y 4
set key bottom left
set size 0.5,0.5
set xrange [16:1024]
#set xtics ("64" 64, "32" 32, "16" 16, "8" 8, "4" 4 )
set terminal postscript eps enhanced color "NimbusSanL-Regu" fontfile "/usr/share/texmf-texlive/fonts/type1/urw/helvetic/uhvr8a.pfb" 14
set output "3depths.eps"

plot "depth12.dat" using 1:(16*1622.956865/($2*$1)) title "Depth = 12" with linespoints lw 3 lt 3, \
"depth10.dat" using 1:(16*104.027352/($2*$1)) title "Depth = 10 " with linespoints lw 3 lt 2,\
 "depth8.dat" using 1:(16*3.917998/($2*$1)) title "Depth = 8" with linespoints lw 3 lt 1


