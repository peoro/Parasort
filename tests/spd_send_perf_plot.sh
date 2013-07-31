#!/bin/bash

platform="pianosa"
pwd=`pwd`
SAVE_DIR=$pwd"/mpi_comm_perf/"$platform


##################################
########### Send Test ############
##################################
cd $SAVE_DIR/send
filename="tsend_4"

plot=( 'set terminal postscript eps enhanced color\n' 'set output "'$SAVE_DIR"/"$filename'.eps"\n' 'set xlabel "Size (bytes)"\n' 'set ylabel "time (us)"\n' 'set title "Send performance for MPI (pianosa) type blocking"\n' 'plot "'$filename'.gpl" using 4:5 title "tsend" with linespoints\n' 'clear\n' )
echo -e ${plot[@]} > $filename".plot"
gnuplot $filename".plot" > /dev/null




filename="tsend_32"

plot=( 'set terminal postscript eps enhanced color\n' 'set output "'$SAVE_DIR"/"$filename'.eps"\n' 'set xlabel "Size (bytes)"\n' 'set ylabel "time (us)"\n' 'set title "Send performance for MPI (pianosa) type blocking"\n' 'plot "'$filename'.gpl" using 4:5 title "tsend" with linespoints\n' 'clear\n' )
echo -e ${plot[@]} > $filename".plot"
gnuplot $filename".plot" > /dev/null


##################################
##### Send Logscale Test #########
##################################
cd $SAVE_DIR/send/logscale
filename="tsend_logscale"

plot=( 'set terminal postscript eps enhanced color\n' 'set output "'$SAVE_DIR"/"$filename'.eps"\n' 'set xlabel "Size (bytes)"\n' 'set ylabel "time (us)"\n' 'set title "Send performance for MPI (pianosa) type blocking"\n' 'plot "'$filename'.gpl" using 4:5 title "tsend" with linespoints\n' 'clear\n' )
echo -e ${plot[@]} > $filename".plot"
gnuplot $filename".plot" > /dev/null


##################################
#### Bisection Logscale Test #####
##################################
prefix="bisect_logscale"
N=( 4 8 16 )
cmd='plot 0 notitle'

for n in ${N[*]}; do
	cd $SAVE_DIR/bisect/logscale

	mkdir -p n$n
	
	filename="n"$n"/"$prefix"_n"$n
	
	cmd=$cmd', "'$filename'.gpl" using 4:5 title "n'$n'" with linespoints'
done

filename=$prefix

plot=( 'set terminal postscript eps enhanced color\n' 'set output "'$SAVE_DIR"/"$filename'.eps"\n' 'set xlabel "Size (bytes)"\n' 'set ylabel "time (us)"\n' 'set title "Bisection performance for MPI (pianosa) type blocking"\n' $cmd"\n" 'clear\n' )
echo -e ${plot[@]} > $filename".plot"
gnuplot $filename".plot" > /dev/null




##################################
######### Broadcast  Test ########
##################################
filename="bcast"

cd $SAVE_DIR/bcast

cat $filename".gpl" | grep "[0-9]" > tmp

cmd='plot "tmp" using 1:2 title "0" with linespoints, "tmp" using 1:3 title "256" with linespoints, "tmp" using 1:4 title "512" with linespoints, "tmp" using 1:5 title "768" with linespoints, "tmp" using 1:6 title "1024" with linespoints'

plot=( 'set terminal postscript eps enhanced color\n' 'set output"'$SAVE_DIR"/"$filename'.eps"\n' 'set xlabel "Number of processors"\n' 'set ylabel "time (us)"\n' 'set title "Broadcast performance for MPI (pianosa) type blocking"\n' $cmd"\n" 'clear\n' )
echo -e ${plot[@]} > $filename".plot"
gnuplot $filename".plot" > /dev/null

rm tmp



echo FINISHED!!!
