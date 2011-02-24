set terminal postscript eps enhanced color
 set output "/home/nico/tmp/spd/mpi_comm_perf/PCM/tsend_logscale.eps"
 set xlabel "Size (bytes)"
 set ylabel "time (us)"
 set title "Send performance for MPI (PCM) type blocking"
 plot "tsend_logscale.gpl" using 4:5 title "tsend" with linespoints
 clear

