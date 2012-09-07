set xlabel "Number of Cores"
set ylabel "Speedup"
#set title "Startup Time vs Number of Cores: Comparison with OpenMPI Startup"
set logscale x 4
set logscale y 4
set key top left
set size 0.6,0.6
set yrange [1:1200]
#set xtics ("64" 64, "32" 32, "16" 16, "8" 8, "4" 4 )
set terminal postscript eps enhanced color "NimbusSanL-Regu" fontfile "/usr/share/texmf-texlive/fonts/type1/urw/helvetic/uhvr8a.pfb" 14
set output "3schemes.eps"

plot "ideal.dat" using 1:2 title "Ideal Speedup" with linespoints lw 3 ,\
"random.dat" using 1:(1524/$2) title "Random LB" with linespoints lw 3, \
"wkstl.dat" using 1:(1524/$2) title "Work Stealing LB" with linespoints lw 3 ,\
"nbr.dat" using 1:(1524/$2) title "Neighbour-based LB" with linespoints lw 3

