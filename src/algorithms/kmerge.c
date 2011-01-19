
/**
 * @file kmerge.c
 *
 * @brief This file contains a parallel version of k-way mergesort, an external-sorting algorithm to sort atomic elements (integers)
 * *
 * @author Fabio Luporini
 * @version 0.0.01
 */

#include <string.h>
#include <queue>

#include "sorting.h"
#include "common.h"

using namespace std;

const int PARAM_K 	= 1;
const int PARAM_MAP = 0;

/**************************************/
/*               STENCIL              */
/**************************************/

//from_who	: return the ranks of the processes from which i RECEIVE data in the current step
void from_who ( int rank, int k, int active_proc, int *dest, int *n_dest )
{
	//riceverò da un numero di processi che è tra il minimo tra k-1 e active_proc
	*n_dest = ( active_proc >= k ) ? k-1 : active_proc-1;

	int i = 0;
	for ( ; i < *n_dest ; i++ )
		dest[i] = ( active_proc >= k ) ? rank + ( 1 + i )*( active_proc / k ) : rank + 1 + i;
}

//to_who: return the rank of the process (node) to which i SEND data in the current step
int to_who ( int rank, int k, int active_proc )
{
	if ( active_proc < k )
		return 0;
	else
		return rank % ( active_proc / k );
}

//do_i_receive: return a number different from 0 if the calling process (node) has to RECEIVE data FROM another process (node) in the current step.
int do_i_receive ( int rank, int k, int active_proc )
{
	return (rank < (active_proc / k) || rank == 0 );
}

//do_i_send: return a number different from 0 if the calling process (node) has to SEND data TO another process (node) in the current step
int do_i_send ( int rank, int k, int active_proc )
{
	return (rank >= (active_proc / k) && rank < active_proc);
}

/**************************************/
/*            ALGORITHMIC             */
/**************************************/


struct Min_val {
	Min_val ( int val, int run_index ) {
		this->val = val;
		this->run_index = run_index;
	}
	int val;
	int run_index;
	bool operator<(const Min_val& m) const {
		return val > m.val;
	}
};

//merges #runs data on a File
void fileFusion ( Data *data_owned, Data *merging, int runs )
{
	fileKMerge ( data_owned, runs, DAL_dataSize ( data_owned ) * runs, merging );
}

//invariant: all buckets have equal size
void memoryFusion ( Data *data_owned, Data *merging, int runs )
{
	priority_queue<Min_val> heap;
	int *runs_indexes 	= (int*) calloc ( sizeof(int), runs );

	//initializing the heap
	for ( int i = 0; i < runs; i++ )
		heap.push ( Min_val ( data_owned[i].array.data[0], i ) );

	//merging
	for ( int i = 0; i < merging->array.size; i++ ) {
		Min_val min = heap.top();
		heap.pop();
		merging->array.data[i] = min.val;

		if ( ++(runs_indexes[min.run_index]) != data_owned[min.run_index].array.size )
			heap.push ( Min_val ( data_owned[min.run_index].array.data[runs_indexes[min.run_index]], min.run_index ) );
	}

	free ( runs_indexes );
}

//invariant: all buckets have equal size
//postcondition: data_owned will be invalid after the merging
Data fusion ( Data *data_owned, int runs )
{
	Data merging;

	DAL_init ( &merging );

	//if the local data is already on File, than the merged will be performed on File too for sure
	switch ( data_owned->medium ) {
		case File: {
			fileFusion ( data_owned, &merging, runs );
			break;
		}
		case Array: {
			if ( DAL_allocData ( &merging, DAL_dataSize ( data_owned ) * runs ))
				memoryFusion ( data_owned, &merging, runs );
			else
				fileFusion ( data_owned, &merging, runs );
			break;
		}
		default:
			DAL_UNSUPPORTED( data_owned );
	}

	//free old Data
	for ( int i = 0; i < runs; i++ )
		DAL_destroy ( data_owned + i );

	return merging;
}

//invariant: given n = #processors, k = #ways, q an integer, it must be n == k * q
void mk_mergesort ( const TestInfo *ti, Data *data_local )
{
	const int 	total_size	= GET_M ( ti );
	const int 	rank 		= GET_ID ( ti );
	const int 	k 			= ti->algoVar[PARAM_K];
	int 		active_proc = GET_N ( ti );

	Data		*data_owned = (Data*) malloc ( sizeof(Data) * k );
	PhaseHandle scatterP, localP, gatherP;

	//initializing datas
	data_owned[0] = *data_local;
	for ( int i = 1; i < k; i ++ )
		DAL_init ( data_owned + i );

	//scattering data partitions
	scatterP = startPhase( ti, "Scattering" );
	DAL_scatter ( data_owned, GET_LOCAL_M ( ti ), 0 );
	stopPhase( ti, scatterP );

	//sorting local partition
	localP = startPhase( ti, "Sorting" );
	sequentialSort ( ti, data_owned );

	//for each merge-step, this array contains the ranks of the k processes from which partitions will be received.
	int *dests = (int*) calloc ( sizeof(int), k-1 );

	while ( active_proc > 1 ) {
		if ( do_i_receive( rank, k, active_proc ) ) {

			//receiving k-1 partitions
			int receiving, n_dests;
			from_who( rank, k, active_proc, dests, &n_dests );
			for ( receiving = 0; receiving < n_dests; receiving++ )
				DAL_receive ( data_owned + receiving + 1, total_size / active_proc, dests[receiving] );

			//fusion phase
			if ( active_proc >= k )
				*data_owned = fusion ( data_owned, k );
			else
				*data_owned = fusion ( data_owned, active_proc ); //case of unbalanced tree, latest step

		}
		else if ( do_i_send ( rank, k, active_proc ) )
			DAL_send ( data_owned, to_who( rank, k, active_proc ));

		//keeps updated the number of processes that partecipate to the sorting in the next step
		active_proc /= k;
	}

	stopPhase( ti, localP );

	//swap
	*data_local = *data_owned;

	//frees memory
	for ( int i = 1; i < k; i++ )
		DAL_destroy ( data_owned + i );
	free ( dests );
	free ( data_owned );
}

extern "C"
{
	void sort ( const TestInfo *ti )
	{
		Data data_local;
		DAL_init ( &data_local );
		mk_mergesort ( ti, &data_local );
		DAL_destroy ( &data_local );
	}

	void mainSort( const TestInfo *ti, Data *data_local )
	{
		mk_mergesort ( ti, data_local );
	}
}

