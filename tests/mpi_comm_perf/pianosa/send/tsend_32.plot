set terminal postscript eps enhanced color
 set output "/home/nico/Projects/SPD_LAB/Project/spd-project/tests/mpi_comm_perf/pianosa/tsend_32.eps"
 set xlabel "Size (bytes)"
 set ylabel "time (us)"
 set title "Send performance for MPI (pianosa) type blocking"
 plot "tsend_32.gpl" using 4:5 title "tsend" with linespoints
 clear

