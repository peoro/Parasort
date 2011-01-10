#!/bin/bash

DATA_SIZE_MB=( 24 29 )
path=nsends_logs

mkdir -p $path

#for each data size (M)
for MB in ${DATA_SIZE_MB[*]}; do
    M=$(( 2 ** $DATA_SIZE_MB ))

    filename=$path/"M"$((2**$MB))"MB"
    rm -f $filename
    touch $filename

    #for each number of sends (s)
    for s in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19
    do
		n_sends=$((2**$s))
        echo "Starting test for M="$M "MB, #sends="$n_sends
        echo -n $((2**$s))" & "$(($M / $n_sends)) >> $filename

        mpiexec -machinefile ../machinefile_pianosa -n 2 ../tsetup $M $n_sends >> $filename

        echo " " >> $filename

        #clog2TOslog2 spdlog.clog2 -o $filename".slog2" &> /dev/null
    done
done


