#!/bin/bash

MEGABYTE=1000000
DATA_SIZE_MB=( 25 100 400 )
N_SENDS=( 1 5 10 20 50 100 )
path=nsends_logs

mkdir -p $path

#for each data size (M)
for MB in ${DATA_SIZE_MB[*]}; do
    M=$(( $MB * $MEGABYTE ))

    filename=$path/"M"$MB"MB"
	touch $filename
	
    #for each number of sends (s)
    for s in ${N_SENDS[*]}; do

    	echo "Starting test for M="$M "MB, #sends="$s
    	echo "Starting test for M="$M "MB, #sends="$s >> $filename	
                
        mpiexec -machinefile ../machinefile_pianosa -n 2 ../tsetup $M $s &>> $filename

		echo " " >> $filename

        #clog2TOslog2 spdlog.clog2 -o $filename".slog2" &> /dev/null    
    done
done

