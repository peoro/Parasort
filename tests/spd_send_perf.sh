#!/bin/bash

platform="pianosa"
pwd=`pwd`
SAVE_DIR=$pwd"/mpi_comm_perf/"$platform

mkdir -p $SAVE_DIR/send/logscale
mkdir -p $SAVE_DIR/bisect/logscale
mkdir -p $SAVE_DIR/bcast/


##################################
########### Send Test ############
##################################
cd $SAVE_DIR/send
filename="tsend_4"

echo "Starting send test from 0 to 32 bytes (by increasing each one of 4 bytes)"
mpiexec -np 2 mpptest -max_run_time 2000 -givedy -size 0 32 4 -gnuplot -fname $filename".plot" > /dev/null


filename="tsend_32"

echo "Starting send test from 0 to 1024 bytes (by increasing each one of 32 bytes)"
mpiexec -np 2 mpptest -max_run_time 2000 -givedy -gnuplot -fname $filename".plot" > /dev/null

##################################
##### Send Logscale Test #########
##################################
cd $SAVE_DIR/send/logscale
filename="tsend_logscale"

echo "Starting send test from 0 to 32768 bytes (by increasing each one of 2^i bytes)"
mpiexec -np 2 mpptest -max_run_time 2000 -givedy -logscale -size 0 32768 2 -gnuplot -fname $filename".plot" > /dev/null


##################################
#### Bisection Logscale Test #####
##################################
prefix="bisect_logscale"
N=( 4 8 16 )

for n in ${N[*]}; do
	cd $SAVE_DIR/bisect/logscale

	mkdir -p n$n
	
	filename="n"$n"/"$prefix"_n"$n

	echo "Starting bisection test with "$n" processors from 0 to 32768 bytes (by increasing each one of 2^i bytes)"
	mpiexec -np $n mpptest -max_run_time 2000 -givedy -bisect -logscale -size 0 32768 2 -gnuplot -fname $filename".plot" > /dev/null
done


##################################
######### Broadcast  Test ########
##################################
filename="bcast"
N=( 4 8 16 )

cd $SAVE_DIR/bcast

for n in ${N[*]}; do
	
	echo "Starting bcast test with "$n" processors from 0 to 1024 bytes (by increasing each one of 256 bytes)"
	mpiexec -np $n goptest -max_run_time 2000 -givedy -bcast -gnuplot -fname $filename".plot" > /dev/null
done


echo FINISHED!!!
