set terminal png size 1149, 861 crop
 set output "/home/nico/tmp/mpi_comm_perf/pianosa/tsend_4.png"
 set xlabel "Size (bytes)"
 set ylabel "time (us)"
 set title "Send performance for MPI (pianosa) type blocking"
 plot "tsend_4.gpl" using 4:5 title "tsend" with linespoints
 clear

