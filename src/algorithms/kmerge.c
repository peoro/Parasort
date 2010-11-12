
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

const int PARAM_K 	= 0;
const int PARAM_MAP 	= 1;

//from_who	: return the ranks of the processes from which i RECEIVE data in the current step
void from_who ( int rank, int k, int active_proc, int *dest ) 
{
	int i = 0;
	for ( ; i < (k-1) ; i++ )
		dest[i] = rank + ( 1 + i )*( active_proc / k );
}

//to_who	: return the rank of the process (node) to which i SEND data in the current step
int to_who ( int rank, int k, int active_proc ) 
{
	return rank % ( active_proc / k );
}

//do_i_receive 	: return a number different from 0 if the calling process (node) has to RECEIVE data FROM another process (node) in the current step.
int do_i_receive ( int rank, int k, int active_proc )
{
	return (rank < (active_proc / k));
}

//do_i_send	: return a number different from 0 if the calling process (node) has to SEND data TO another process (node) in the current step
int do_i_send ( int rank, int k, int active_proc )
{
	return (rank >= (active_proc / k));
}


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
	
void fusion ( int *sorting, int left, int right, int size, int* merging) 
{
	priority_queue<Min_val> heap;
	int i, runs = (right - left) / size; //== k
	int *runs_indexes = (int*) calloc ( sizeof(int), runs );   
	
	//initializing the heap
	for ( i = 0; i < runs; i++ )
		heap.push ( Min_val ( sorting[i*size], i ) );
	
	//merging
	for ( i = 0; i < right; i++ ) {
		Min_val min = heap.top();
		heap.pop();
		merging[i] = min.val;
		if ( ++(runs_indexes[min.run_index]) != size ) 	
			heap.push ( Min_val ( sorting[min.run_index*size + runs_indexes[min.run_index]], min.run_index ) );
	}
	
	//final copy
	for ( i = 0; i < right; i++ )
		sorting[i] = merging[i]; 
	
	free ( runs_indexes );
}

//invariant: given n = #processors, k = #ways, q an integer, it must be n == k^q
void mk_mergesort ( const TestInfo *ti, int *sorting )
{
	const int total_size 	= GET_M ( ti );
	int size 		= GET_LOCAL_M ( ti );
	int rank		= GET_ID ( ti );
	int active_proc 	= GET_N ( ti );

	int k = ti->algoVar[PARAM_K]; 
	int merging [ total_size ]; 

	MPI_Status stat;

	//scattering data partitions
	MPI_Scatter ( sorting, size, MPI_INT, sorting, size, MPI_INT, 0, MPI_COMM_WORLD );
	//sorting local partition
	qsort ( sorting, size, sizeof(int), compare );

	//for each merge-step, this array contains the ranks of the k processes from which partitions will be received. 
	int *dests = (int*) calloc ( sizeof(int), k-1 ); 
	int j = 0;
	for ( ; j < _logk(GET_N(ti), k); ++ j ) {
		if ( do_i_receive( rank, k, active_proc ) ) {
			//receiving k-1 partitions
			int receiving = 0;
			from_who( rank, k, active_proc, dests );
			for ( ; receiving < (k-1); receiving++ ) 
				MPI_Recv ( (int*)sorting + (receiving+1)*size, size, MPI_INT, dests[receiving] , 0, MPI_COMM_WORLD, &stat );
								
			//fusion phase
			fusion ( sorting, 0, size + (total_size * (k-1) / active_proc), size, merging );
		
			size += ( total_size * (k-1) / active_proc ); //size of the ordered sequence
		}
		if ( do_i_send ( rank, k, active_proc ) )
			MPI_Send ( sorting, size, MPI_INT, to_who( rank, k, active_proc ), 0, MPI_COMM_WORLD );
		
		active_proc /= k;
	}
	
	free ( dests );
}

extern "C" 
{
	void sort ( const TestInfo *ti )
	{
		int sorting [GET_M ( ti )]; 
		mk_mergesort ( ti, sorting );
	}

	void mainSort( const TestInfo *ti, int *sorting, long size )
	{	
		mk_mergesort ( ti, sorting );	
	}
}

