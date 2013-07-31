#!/bin/bash

#args=( $@ )
platform=pianosa
testID=00
SAVE_DIR="logs/test_"$testID"_"$platform

ALGOS=( quicksort mergesort kmerge lbmergesort lbkmergesort samplesort bucketsort bitonicsort )
NUM_PROCESSORS=( 1 2 4 8 16 )

SEEDS=( 1 2 3 )
NUM_TEST=3

let KILO=1024
let MEGA=KILO*KILO
let GIGA=MEGA*KILO

let size=MEGA
for i in `seq 0 10`; do
	let DATA_SIZE[$i]=size
	let size=2*size
done


path=$SAVE_DIR/
mkdir -p $path

date

for t in `seq 1 $NUM_TEST`; do
	#for each data size (M)
	for M in ${DATA_SIZE[*]}; do
		let bytes=M*4
		let kilobytes=bytes/1024
		let megabytes=kilobytes/1024
		let gigabytes=megabytes/1024


		if [ $gigabytes -ne 0 ]; then
			val=$gigabytes
			measure=GB
		else if [ $megabytes -ne 0 ]; then
				val=$megabytes
				measure=MB
			else
				val=$kilobytes
				measure=KB
			fi
		fi

		algoVar[0]=0
		algoVar[1]=4
		for n in ${NUM_PROCESSORS[*]}; do

			#setting algoVar[1] for kmerge
			if [[ $n -eq 2 ]]; then
				algoVar[1]=2
			else
				algoVar[1]=4
			fi

			#for each seed (s)
			for s in ${SEEDS[*]}; do

				if [ $n -ne 1 ]; then
					for algo in ${ALGOS[*]}; do
						echo "Starting test "$t" for M="$M "integers (="$val $measure"), n="$n", seed="$s", a="$algo "> ""result""_n"$n"_M"$M"_"$algo"_""s"$s"_t"$t

						filename=$path/"result""_n"$n"_M"$M"_"$algo"_""s"$s"_t"$t
						touch $filename
						mpiexec -np $n ~/spd-project/src/spd -M $M -s $s -a $algo -1 ${algoVar[0]} -2 ${algoVar[1]} &> $filename
						for machine in $(mpdtrace); do ssh $machine "rm /l/disc1/spd/tmp*" &> /dev/null; done

						#clog2TOslog2 spdlog.clog2 -o $filename".slog2" &> /dev/null
					done
				else
					algo=sequential
					echo "Starting test "$t" for M="$M "integers (="$val $measure"), n="$n", seed="$s", a="$algo "> ""result""_n"$n"_M"$M"_"$algo"_""s"$s"_t"$t

					filename=$path/"result""_n"$n"_M"$M"_"$algo"_""s"$s"_t"$t
					touch $filename
					mpiexec -np $n ~/spd-project/src/spd -M $M -s $s -a $algo &> $filename
					for machine in $(mpdtrace); do ssh $machine "rm /l/disc1/spd/tmp*" &> /dev/null; done

					#clog2TOslog2 spdlog.clog2 -o $filename".slog2" &> /dev/null
				fi
				#rm ~/.spd/data/*.sorted
			done
		done
		rm ~/.spd/data/*.unsorted
	done
done

date

echo FINISHED!!!
