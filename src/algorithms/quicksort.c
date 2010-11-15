
/**
 * @file quicksort.c
 *
 * @brief This file contains a parallel version of quicksort, a standard algorithm to sort atomic elements (integers) 
 *
 * To scatter data:
 *   each node will call \scatter, which takes an array \array of \size=M elements.
 *   when \scatter returns, only the first \size items of \array will be valid:
 *     \array will contain the partition each node must sort.
 * To gather back data:
 *   each node, but node 0, sends its sorted partition to node 0.
 *   node 0 will sequentially read data from any node, into the original array,
 *     (at the right offset) so that no merging will be required.
 *
 *
 * @author Paolo Giangrandi
 * @version 0.0.01
 */

#include "sorting.h"
#include "utils.h"
#include "string.h"

#include <time.h> // srand( time(0) )
#include <assert.h>

/**************************************/
/*               STENCIL              */
/**************************************/

#define ACTIVE_PROCS(ti,step) _pow2(step)
#define GET_STEP_COUNT(ti) _log2( GET_N(ti) )

//from_who	: return the rank of the process (node) from which i RECEIVE data in the current step
int from_who( const TestInfo *ti, int step ) 
{
	switch( ti->algoVar[0] ) {
		case 1: return GET_ID(ti) - ACTIVE_PROCS(ti,step);
	}
	return GET_ID(ti) - ( GET_N(ti) / ACTIVE_PROCS(ti,step+1) );
}

//to_who	: return the rank of the process (node) from which i SEND data in the current step
int to_who( const TestInfo *ti, int step ) 
{
	switch( ti->algoVar[0] ) {
		case 1: return GET_ID(ti) + ACTIVE_PROCS(ti,step);
	}
	return GET_ID(ti) + ( GET_N(ti) / ACTIVE_PROCS(ti,step+1) );
}

//do_i_send	: true if the calling process (node) has to SEND data TO another process (node) in the current step
bool do_i_send( const TestInfo *ti, int step )
{
	switch( ti->algoVar[0] ) {
		case 1: return GET_ID(ti) < ACTIVE_PROCS(ti,step);
	}
	return ! ( GET_ID(ti) % ( GET_N(ti) / ACTIVE_PROCS(ti,step) ) );
}

//do_i_receive 	: return a number different from 0 if the calling process (node) has to RECEIVE data FROM another process (node) in the current step.
bool do_i_receive( const TestInfo *ti, int step )
{
	switch( ti->algoVar[0] ) {
		case 1: return ! do_i_send( ti, step ) && GET_ID(ti) < 2*ACTIVE_PROCS(ti,step);
	}
	return ! do_i_send( ti, step ) && ! ( GET_ID(ti) % ( GET_N(ti) / ACTIVE_PROCS(ti,step+1) ) ) ;
}

// return the ID of the node owning the n-th data token. Useful during gathering
int nth_token_owner( const TestInfo *ti, int n )
{
	switch( ti->algoVar[0] ) {
		case 1:
		{
			int res = 0;
			int nodes = GET_N(ti);
			int steps = GET_STEP_COUNT(ti);
			int i;
			
			for( i = 0; i < steps; ++ i ) {
				if( n >= nodes/2 ) {
					res += ACTIVE_PROCS(ti,i);
					n -= nodes/2;
				}
				nodes /= 2;
			}
			
			return res;
		}
	}
	return n;
}



/**************************************/
/*            ALGORITHMIC             */
/**************************************/

#define SWAP( a, i, j ) { int tmp; tmp = a[i]; a[i] = a[j]; a[j] = tmp; }

// partitions the array:
// a is the array, of size size. p is the pivot
long _partition( int *a, long size, int p )
{
	long i = 0, j = size-1;
	
	while( true ) {
		while( a[i] < p ) { ++ i; }
		while( a[j] > p ) { -- j; }
		if( i >= j ) {
			break;
		}
		SWAP( a, i, j );
	}
	
	return j;
}
long partition( int *a, long size )
{
	return _partition( a, size, a[ rand()%size ] );
}

void scatter( const TestInfo *ti, int *a, long *size )
{
	int step = 0;
	
	for( step = 0; step < GET_STEP_COUNT(ti); ++ step ) {
		if( do_i_send(ti,step) ) {
			long lim = partition( a, *size );
			// printf( "%d->%d:Partition(%ld): %ld\n", GET_ID(ti), to_who(ti,step), *size, lim );
			_MPI_Send ( & a[lim], *size-lim, MPI_INT, to_who( ti, step ), 0, MPI_COMM_WORLD );
			*size = lim;
		}
		if( do_i_receive(ti,step) ) {
			MPI_Status stat;
			_MPI_Recv( a, GET_M(ti), MPI_INT, from_who( ti, step ), 0, MPI_COMM_WORLD, &stat );
			_MPI_Get_count( &stat, MPI_INT, size );
		}
	}
	
	// printf( "Node %d has got %ld data\n", GET_ID(ti), *size );
}

void gather( const TestInfo *ti, int *a, long *size )
{
	// node 0
	if( ! GET_ID(ti) ) {
		int actualSize = *size;
		int i;
		// receiving sequentially from ohter nodes
		for( i = 1; i < GET_N(ti); ++ i ) {
			MPI_Status stat;
			_MPI_Recv( a+actualSize, GET_M(ti)-actualSize, MPI_INT, nth_token_owner(ti,i), 0, MPI_COMM_WORLD, &stat );
			_MPI_Get_count( &stat, MPI_INT, size );
			actualSize += *size;
		}
		*size = actualSize;
	}
	// other nodes
	else {
		_MPI_Send ( a, *size, MPI_INT, 0, 0, MPI_COMM_WORLD );
		*size = 0;
	}
}


/**************************************/
/*              SORTING               */
/**************************************/

void actualSort( const TestInfo *ti, int *a, long *size )
{
	srand( time(0) );
	
	scatter( ti, a, size );
	qsort ( a, *size, sizeof(int), compare );
	gather( ti, a, size );
}

void sort( const TestInfo *ti )
{
	long size = GET_M(ti);
	int *a = (int*) malloc( sizeof(int)*size );
	
	actualSort( ti, a, &size );
	assert( ! size );
	
	free( a );
}

void mainSort( const TestInfo *ti, int *a, long size )
{	
	actualSort( ti, a, &size );
	assert( size == GET_M(ti) );
}

