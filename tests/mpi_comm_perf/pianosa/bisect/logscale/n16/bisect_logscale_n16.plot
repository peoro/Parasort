set xlabel "Size (bytes)"
set ylabel "time (us)"
set logscale xy
set title "Comm Perf for MPI (u3) type blocking-bisect"
plot 'n16/bisect_logscale_n16.gpl' using 4:5:7:8 notitle with errorlines
pause -1 "Press <return> to continue"
clear
