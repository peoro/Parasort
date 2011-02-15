#!/bin/bash

DATA_SIZE_MB=( 32 )
PAR_DEGREES=( 2 4 8 16 )
N_SENDS=( 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 )
path=nsends_logs
RET=""

function byteMe() { # Divides by 2^10 until < 1024 and then append metric suffix
declare -a METRIC=('' 'K' 'M' 'G' 'T' 'X' 'P') # Array of suffixes
MAGNITUDE=0  # magnitude of 2^10
PRECISION="scale=0" # change this numeric value to inrease decimal precision
UNITS=$1 # numeric paramerter val (in bytes) to be converted
while [ "${UNITS/.*}" -gt 1024 ]; do # compares integers (b/c no floats in bash)
	UNITS=`echo "$PRECISION; $UNITS/1024" | bc` # floating point math via `bc`
	((MAGNITUDE++)) # increments counter for array pointer
done
RET=$UNITS${METRIC[$MAGNITUDE]}
}

mkdir -p $path

#for each data size (M)
for MB in ${DATA_SIZE_MB[*]}; do
    M=$(( $MB * 1024 * 1024 ))
	
	for N in ${PAR_DEGREES[*]}; do
		filename=$path/"M"$MB"MB_n"$N
		rm -f $filename
		touch $filename

		#for each number of sends (s)
		for s in ${N_SENDS[*]}; do
			n_sends=$((2**$s))
			
			byteMe $(($M / $n_sends))
			
		    echo "Starting test for M="$M", n="$N", #sends="$n_sends
		    echo -n $((2**$s))" & "$RET" & " >> $filename

		    mpiexec -n $N ./tsetup $M $n_sends | awk '{printf $1 " & " $2 " & " $3}' >> $filename

		    echo " \\\\\\hline" >> $filename

		    #clog2TOslog2 spdlog.clog2 -o $filename".slog2" &> /dev/null
		done
	done
done


