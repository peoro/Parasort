#!/bin/bash

[[ -n "$1" ]] || { echo "Usage: $0 \"path\""; exit 0 ; }

path=$1
platform=`echo $path | cut -d "_" -f3 | cut -d "/" -f1`
testID=`echo $path | cut -d "_" -f2 | cut -d "/" -f1`
output_path="plots/test_"$testID"_"$platform

mkdir -p $output_path

# TODO: handle more seeds and number of tests
s=1
t=1

ALGOS=( `ls $path | grep result_ | cut -d "_" -f4 | sort -u` )
# echo -e ${ALGOS[@]}
DATA_SIZES=( `ls $path | grep result_ | cut -d "_" -f3 | cut -d "M" -f2 | sort -n -u` )
# echo -e ${DATA_SIZES[@]}


#Moving file related to sequential sort to obtain better plots
NUM_PROCS=( `ls $path | grep result_ | cut -d "_" -f2 | cut -d "n" -f2 | sort -n -u` )
# echo -e ${NUM_PROCS[0]}
if [ ${NUM_PROCS[0]} -eq 1 ]; then
	for M in ${DATA_SIZES[*]}; do
		for n in ${NUM_PROCS[*]}; do
			if [ $n -ne 1 ]; then
				cp $path"/result_n1_M"$M"_sequential_s"$s"_t"$t $path"/result_n"$n"_M"$M"_sequential_s"$s"_t"$t
			fi
		done
		rm $path"/result_n1_M"$M"_sequential_s"$s"_t"$t
	done
fi



for algo in ${ALGOS[*]}; do
	NUM_PROCS=( `ls $path | grep result_ | grep "_"$algo"_" | cut -d "_" -f2 | cut -d "n" -f2 | sort -n -u` )
	# echo -e ${NUM_PROCS[@]}

	#init data files for this algo
	for M in ${DATA_SIZES[*]}; do
		echo -e "#n""\t""sort" > $algo"_M"$M".data"
	done
	for n in ${NUM_PROCS[*]}; do
		echo -e "#M""\t""sort" > $algo"_n"$n".data"
	done

	#setting data files for this algo
	#for each M
	for M in ${DATA_SIZES[*]}; do
		#for each n
		for n in ${NUM_PROCS[*]}; do

			file=$path"/result_n"$n"_M"$M"_"$algo"_s"$s"_t"$t

			if [ -f $file ]; then
				sort_time=`python results.py $file`
				echo -e $n"\t"$sort_time >> $algo"_M"$M".data"
				echo -e $M"\t"$sort_time >> $algo"_n"$n".data"
			else
				echo $file" doesn't exist";
			fi

		done
	done

    mkdir -p $output_path"/NxTxM"
	##################################################### NxTxM plot ##########################################################
	terminal='set terminal postscript eps enhanced color\n'
	terminal_name='set output "'$output_path'/NxTxM/'$algo'_'$platform'_NxTxM.eps"\n'
	xtics='set log x\n set xtics (2,4,8,16)\n'
	ytics='' #'set ytics 0,2e7,2.2e8\n'
	yrange='' #'set yrange [0:2.2e8]\n'
	xlabel='set xlabel "Number of processors"\n'
	ylabel='set ylabel "Execution Time (microsecs)"\n'
	title='set title "Performance for '$algo' (\"'$platform'\")"\n'
	plot='plot 0 notitle'
	gpl_content=( $terminal $terminal_name $xtics $ytics $yrange $xlabel $ylabel $title $plot )
	gpl_filename=$algo"_NxTxM.gpl"

	echo -en ${gpl_content[*]} > $gpl_filename
	for M in ${DATA_SIZES[*]}; do
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
		echo -n ', "'$algo'_M'$M'.data" using 1:2 with linespoints title "'$val' '$measure'"' >> $gpl_filename
	done
	gnuplot ""$gpl_filename




    mkdir -p $output_path"/MxTxN"
	##################################################### MxTxN plot ##########################################################
	terminal='set terminal postscript eps enhanced color\n'
	terminal_name='set output "'$output_path'/MxTxN/'$algo'_'$platform'_MxTxN.eps"\n'
	xtics='' #'set log x\n set xtics (2,4,8,16)\n'
	ytics='' #'set ytics 0,2e7,2.2e8\n'
	yrange='' #'set yrange [0:2.2e8]\n'
	xlabel='set xlabel "Number of integers"\n'
	ylabel='set ylabel "Execution Time (microsecs)"\n'
	title='set title "Performance for '$algo' (\"'$platform'\")"\n'
	plot='plot 0 notitle'
	gpl_content=( $terminal $terminal_name $xtics $ytics $yrange $xlabel $ylabel $title $plot )
	gpl_filename=$algo"_MxTxN.gpl"

	echo -en ${gpl_content[*]} > $gpl_filename
	for n in ${NUM_PROCS[*]}; do
		echo -n ', "'$algo'_n'$n'.data" using 1:2 with linespoints title "'$n' processors"' >> $gpl_filename
	done
	gnuplot ""$gpl_filename
done









mkdir -p $output_path"/MxTxA"
##################################################### MxTxA plots ##########################################################
NUM_PROCS=( `ls $path | grep result_ | grep -v _sequential_ | cut -d "_" -f2 | cut -d "n" -f2 | sort -n -u` )
for n in ${NUM_PROCS[*]}; do
	terminal='set terminal postscript eps enhanced color\n'
	terminal_name='set output "'$output_path'/MxTxA/n'$n'_'$platform'_MxTxA.eps"\n'
	xtics='' #'set log x\n set xtics (2,4,8,16)\n'
	ytics='' #'set ytics 0,2e7,2.2e8\n'
	yrange='' #'set yrange [0:2.2e8]\n'
	xlabel='set xlabel "Number of integers"\n'
	ylabel='set ylabel "Execution Time (microsecs)"\n'
	title='set title "Performance with '$n' processors (\"'$platform'\")"\n'
	plot='plot 0 notitle'
	gpl_content=( $terminal $terminal_name $xtics $ytics $yrange $xlabel $ylabel $title $plot )
	gpl_filename="n"$n"_MxTxA.gpl"

	echo -en ${gpl_content[*]} > $gpl_filename

	for algo in ${ALGOS[*]}; do
		echo -n ', "'$algo'_n'$n'.data" using 1:2 with linespoints title "'$algo'"' >> $gpl_filename
	done
	gnuplot ""$gpl_filename
done






mkdir -p $output_path"/NxTxA"
##################################################### NxTxA plots ##########################################################
for M in ${DATA_SIZES[*]}; do

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

	terminal='set terminal postscript eps enhanced color\n'
	terminal_name='set output "'$output_path'/NxTxA/M'$M'_'$platform'_NxTxA.eps"\n'
	xtics='set log x\n set xtics (2,4,8,16)\n'
	ytics='' #'set ytics 0,2e7,2.2e8\n'
	yrange='' #'set yrange [0:2.2e8]\n'
	xlabel='set xlabel "Number of processors"\n'
	ylabel='set ylabel "Execution Time (microsecs)"\n'
	title='set title "Performance with '$val' '$measure' (\"'$platform'\")"\n'
	plot='plot 0 notitle'
	gpl_content=( $terminal $terminal_name $xtics $ytics $yrange $xlabel $ylabel $title $plot )
	gpl_filename="M"$M"_NxTxA.gpl"

	echo -en ${gpl_content[*]} > $gpl_filename

	for algo in ${ALGOS[*]}; do
		echo -n ', "'$algo'_M'$M'.data" using 1:2 with linespoints title "'$algo'"' >> $gpl_filename
	done
	gnuplot ""$gpl_filename
done

rm *.data
rm *.gpl
