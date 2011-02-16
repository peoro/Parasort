#!/bin/bash

if [[ $# -ne 2 ]]; then
	echo "Usage: $0 \"path\" \"output_path\""
	exit 0 
fi

path=$1
platform=`echo $path | cut -d "_" -f3 | cut -d "/" -f1`
output_path=$2

mkdir -p $output_path

ALGOS=( `ls $path | grep result_ | cut -d "_" -f4 | sort -u` )
# echo -e ${ALGOS[@]}
DATA_SIZES=( 1073741824 )
# echo -e ${DATA_SIZES[@]}
SEEDS=( `ls $path | grep result_ | cut -d "_" -f5 | cut -d "s" -f2 | sort -n -u` )
# echo -e ${SEEDS[@]}
NUM_TEST=( `ls $path | grep result_ | cut -d "_" -f6 | cut -d "t" -f2 | sort -n -u` )
# echo -e ${NUM_TEST[@]}


#Moving file related to sequential sort to obtain better plots
NUM_PROCS=( `ls $path | grep result_ | cut -d "_" -f2 | cut -d "n" -f2 | sort -n -u` )
# echo -e ${NUM_PROCS[0]}
if [ ${NUM_PROCS[0]} -eq 1 ]; then
	for M in ${DATA_SIZES[*]}; do					
		#for each seed
		for s in ${SEEDS[*]}; do
			for t in ${NUM_TEST[*]}; do
				for n in ${NUM_PROCS[*]}; do	
					if [ $n -ne 1 ]; then
						cp $path"/result_n1_M"$M"_sequential_s"$s"_t"$t $path"/result_n"$n"_M"$M"_sequential_s"$s"_t"$t
					fi					
				done
				rm $path"/result_n1_M"$M"_sequential_s"$s"_t"$t
			done
		done
	done
fi

declare -A ALGO_PHASES
declare -a sum_phase_time

for algo in ${ALGOS[*]}; do
	NUM_PROCS=( `ls $path | grep result_ | grep "_"$algo"_" | cut -d "_" -f2 | cut -d "n" -f2 | sort -n -u` )
	# echo -e ${NUM_PROCS[@]}

	file=$path"/result_n"${NUM_PROCS[0]}"_M"${DATA_SIZES[0]}"_"$algo"_s"${SEEDS[0]}"_t"${NUM_TEST[0]}
	phases=(`cat $file | grep Phase | cut -d '"' -f2 | tr " " "_"`)
	ALGO_PHASES[$algo]="`echo ${phases[@]}`"	
# 	echo ${ALGO_PHASES[$algo]}

	#setting data files for this algo
	#for each M
	for M in ${DATA_SIZES[*]}; do
		#for each n
		for n in ${NUM_PROCS[*]}; do
		
			
# 			echo -e $n"\n"
						
			#for each phase
			for p in ${phases[*]}; do						
				let valid_files=0

				let last=n-1
				for rank in `seq 0 $last`; do
					let sum_phase_time[$rank]=0
				done
				
# 				echo $p

				p=`echo $p | tr "_" " "`

				#for each seed
				for s in ${SEEDS[*]}; do

					for t in ${NUM_TEST[*]}; do

						file=$path"/result_n"$n"_M"$M"_"$algo"_s"$s"_t"$t

						if [ -f $file ]; then
							phase_time=(`python results.py "$p" $file 2> /dev/null`)
# 							echo ${phase_time[@]}

							if ! [[ "${phase_time[0]}" =~ ^[0-9]+$ ]] ; then
								echo $file" is not valid!"
							else
								let valid_files=valid_files+1								
								let last=n-1

								for rank in `seq 0 $last`; do
									let sum_phase_time[$rank]=sum_phase_time[$rank]+phase_time[$rank]
								done
							fi
						else
							echo $file" doesn't exist";
						fi
					done
				done

				p=`echo $p | tr " " "_"`

				if [ $valid_files -ne 0 ]; then							
					let last=n-1

					for rank in `seq 0 $last`; do
						let average_phase_time=sum_phase_time[$rank]/valid_files
# 						echo -e $n"\t"$average_phase_time >> $algo"_M"$M"_"$p"_rank"$rank".data"
# 						echo -e $M"\t"$average_phase_time >> $algo"_n"$n"_"$p"_rank"$rank".data"
						echo -e $rank"\t"$average_phase_time >> "n"$n"_M"$M"_"$algo"_"$p".data"
					done
				fi
			done

			mkdir -p $output_path"/RxTxP"
################################################# RxTxP plots ########################################################
			let bytes=M*4
			let kilobytes=bytes/1024
			let megabytes=kilobytes/1024
			let gigabytes=megabytes/1024

			if [ $gigabytes -ne 0 ]; then
				val=$gigabytes
				measure=GB
			else
				if [ $megabytes -ne 0 ]; then
					val=$megabytes
					measure=MB
				else
					val=$kilobytes
					measure=KB
				fi
			fi

			let tmp=last-1
			if [ $algo != "sequential" ]; then
				terminal='set terminal postscript eps enhanced color\n'
				terminal_name='set output "'$output_path'/RxTxP/''n'$n'_M'$M'_'$algo'_'$platform'_RxTxP.eps"\n'
				xtics='set xtics ('`seq 0 $tmp | tr "\n" "," `$last')\n'
				ytics='' #'set ytics 0,2e7,2.2e8\n'
				yrange='' #'set yrange [0:2.2e8]\n'
				xlabel='set xlabel "Ranks"\n'
				ylabel='set ylabel "Execution Time (microsecs)"\n'
				style='set boxwidth 1 absolute\nset style fill solid 1.00 border 1\nset style data histogram\nset style histogram cluster gap 1\n'
				title='set title "Performance of different phases for '$algo' on '$val' '$measure' with '$n' processors (\"'$platform'\")"\n'
				plot='plot 0 notitle'
				gpl_content=( $terminal $terminal_name $xtics $ytics $yrange $xlabel $ylabel $style $title $plot )
				gpl_filename="n"$n"_M"$M"_"$algo"_RxTxP.gpl"

				echo -en ${gpl_content[*]} > $gpl_filename
				
				phases=( `echo ${ALGO_PHASES[$algo]} | tr " " "\n"` )
										
				for p in ${phases[*]}; do
					if [ $p != "spd" ]; then
						echo -n ', "''n'$n'_M'$M'_'$algo'_'$p'.data" using 2 title "'`echo $p | tr "_" " "`'"' >> $gpl_filename
					fi
				done
				gnuplot ""$gpl_filename
			fi			
################################################# END RxTxP plots ########################################################

		done
	done


done

rm *.data
rm *.gpl
