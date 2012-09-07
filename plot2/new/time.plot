set xlabel "Number of Cores"
set ylabel "Time (s)"
set title " "
#Startup Time vs Number of Cores: Comparison with OpenMPI Startup"
set logscale x 2
set logscale y 2
#set logscale y 10
#set key top left
set size 0.7,0.7
#set xtics ("64" 64, "32" 32, "16" 16, "8" 8, "4" 4 )
set terminal postscript eps enhanced color "NimbusSanL-Regu" fontfile "/usr/share/texmf-texlive/fonts/type1/urw/helvetic/uhvr8a.pfb" 14
set output "new.eps"

plot "1.dat" using 1:2 title "Game 6 Kbot-Darkboard 2006 (5.1 million states)" with linespoints lw 3 lt 2, \
"2.dat" using 1:2 title "Virgil-David 1969 (764K states)" with linespoints lw 3 lt 1, \
"1.dat" using 1:(177/$1) title "Ideal Scaling" with linespoints lw 3 lt 3, \
"2.dat" using 1:(612/$1)  notitle with linespoints lw 3 lt 3

