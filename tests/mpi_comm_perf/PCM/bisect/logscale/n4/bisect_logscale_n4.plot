set xlabel "Size (bytes)"
set ylabel "time (us)"
set logscale xy
set title "Comm Perf for MPI (intel-00.hpc.acer.pisa) type blocking-bisect"
plot '/data/students1/mpi_comm_perf/PCM/bisect/logscale/n4/bisect_logscale_n4.gpl' using 4:5:7:8 notitle with errorlines
pause -1 "Press <return> to continue"
clear
