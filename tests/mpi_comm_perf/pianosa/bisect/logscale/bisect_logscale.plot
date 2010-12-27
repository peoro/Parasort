set terminal png size 1149, 861 crop
 set output "/home/nico/tmp/prova2/mpi_comm_perf/pianosa/bisect_logscale.png"
 set xlabel "Size (bytes)"
 set ylabel "time (us)"
 set title "Bisection performance for MPI (pianosa) type blocking"
 plot 0 notitle, "n4/bisect_logscale_n4.gpl" using 4:5 title "n4" with linespoints, "n8/bisect_logscale_n8.gpl" using 4:5 title "n8" with linespoints, "n16/bisect_logscale_n16.gpl" using 4:5 title "n16" with linespoints
 clear

