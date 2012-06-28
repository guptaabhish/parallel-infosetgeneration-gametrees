set xlabel "Number of Cores"
set ylabel "Time (s)"
#set title "Startup Time vs Number of Cores: Comparison with OpenMPI Startup"
set logscale x 4
#set logscale y 10
#set key top left
set size 0.75,0.75
#set xtics ("64" 64, "32" 32, "16" 16, "8" 8, "4" 4 )
set terminal postscript eps enhanced color "NimbusSanL-Regu" fontfile "/usr/share/texmf-texlive/fonts/type1/urw/helvetic/uhvr8a.pfb" 14
set output "3depths.eps"

plot "depth8.dat" using 1:2 title "Depth = 8" with linespoints lw 3, \
"depth10.dat" using 1:2 title "Depth = 10 " with linespoints lw 3 ,\
"depth12.dat" using 1:2 title "Depth = 12" with linespoints lw 3 

