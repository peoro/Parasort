
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
#include "utils.h"

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
	
//invariant: all buckets have equal size 
void fusion ( Data *data_owned, int runs ) 
{
	priority_queue<Min_val> heap;
	int 	*runs_indexes 	= (int*) calloc ( sizeof(int), runs );   
	int 	*merging 		= (int*) malloc ( sizeof(int) * data_owned->size * runs ); /*TODO: needs to be limited somehow to the memory size*/;  
	int 	i;

	//initializing the heap
	for ( i = 0; i < runs; i++ )
		heap.push ( Min_val ( data_owned[i].array[0], i ) );
	
	//merging
	for ( i = 0; i < data_owned->size * runs; i++ ) {
		Min_val min = heap.top();
		heap.pop();
		merging[i] = min.val;
		
		if ( ++(runs_indexes[min.run_index]) != data_owned[min.run_index].size ) 	
			heap.push ( Min_val ( data_owned[min.run_index].array[runs_indexes[min.run_index]], min.run_index ) );
	}
	
	//realloc + final copy
	reallocDataArray ( data_owned, data_owned->size * runs );
	for ( i = 0; i < data_owned->size; i++ )
		data_owned->array[i] = merging[i]; 
	
	free ( runs_indexes );
	free ( merging );
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

	data_owned[0] = *data_local;	
	
	//scattering data partitions
	scatterP = startPhase( ti, "Scattering" );
	scatter ( ti, data_owned, GET_LOCAL_M ( ti ), 0 );
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
				receive ( ti, data_owned + receiving + 1, total_size / active_proc, dests[receiving] );
				
			//fusion phase
			if ( active_proc >= k )
				fusion ( data_owned, k );
			else 
				fusion ( data_owned, active_proc ); //case of unbalanced tree, latest step
				
		}
		else if ( do_i_send ( rank, k, active_proc ) ) 
			send ( ti, data_owned, to_who( rank, k, active_proc ));
		
		//keeps updated the number of processes that partecipate to the sorting in the next step
		active_proc /= k;
	}
	
	stopPhase( ti, localP );
	//TODO: swap need to be handled by framework
	data_local->array = data_owned->array;
	data_local->size = data_owned->size;
	
	//frees memory
	int i;
	for ( i = 1; i < k; i++ )
		destroyData ( data_owned + i );
	free ( dests );
}

extern "C" 
{
	void sort ( const TestInfo *ti )
	{
		Data data_local;
		mk_mergesort ( ti, &data_local );
		destroyData ( &data_local );
	}

	void mainSort( const TestInfo *ti, Data *data_local )
	{	
		mk_mergesort ( ti, data_local );
	}
}

