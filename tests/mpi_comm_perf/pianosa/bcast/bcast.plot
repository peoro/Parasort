set terminal png size 1149, 861 crop
 set output "/home/nico/tmp/prova2/mpi_comm_perf/pianosa/bcast.png"
 set xlabel "Size (bytes)"
 set ylabel "time (us)"
 set title "Broadcast performance for MPI (pianosa) type blocking"
 plot "tmp" using 1:2 title "0" with linespoints, "tmp" using 1:3 title "256" with linespoints, "tmp" using 1:4 title "512" with linespoints, "tmp" using 1:5 title "768" with linespoints, "tmp" using 1:6 title "1024" with linespoints
 clear

