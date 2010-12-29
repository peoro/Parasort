set terminal postscript eps enhanced color
 set output "/home/nico/Projects/SPD_LAB/Project/spd-project/tests/mpi_comm_perf/pianosa/bcast.eps"
 set xlabel "Size (bytes)"
 set ylabel "time (us)"
 set title "Broadcast performance for MPI (pianosa) type blocking"
 plot "tmp" using 1:2 title "0" with linespoints, "tmp" using 1:3 title "256" with linespoints, "tmp" using 1:4 title "512" with linespoints, "tmp" using 1:5 title "768" with linespoints, "tmp" using 1:6 title "1024" with linespoints
 clear

