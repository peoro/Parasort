#!/bin/bash

DATA_SIZE_MB=( 29 )
path=nsends_logs

mkdir -p $path

#for each data size (M)
for MB in ${DATA_SIZE_MB[*]}; do
    M=$(( 2 ** $DATA_SIZE_MB ))

    filename=$path/"M"$MB"MB"
	touch $filename
	
    #for each number of sends (s)
    for s in {0..19}; do

    	echo "Starting test for M="$M "MB, #sends="$s
    	echo "Starting test for M="$M "MB, #sends="$s >> $filename	
                
        mpiexec -machinefile ../machinefile_pianosa -n 2 ../tsetup $M $((2**$s)) >> $filename

		echo " " >> $filename

        #clog2TOslog2 spdlog.clog2 -o $filename".slog2" &> /dev/null    
    done
done

