
/**
 * @file mergesort.c
 *
 * @brief This file contains a parallel version of mergesort, a standard algorithm to sort atomic elements (integers) 
 * *
 * @author Fabio Luporini
 * @version 0.0.01
 */

#include "sorting.h"
#include "utils.h"
#include "string.h"

#define ACTIVE_PROCS(ti,step) ( GET_N(ti) / _pow2( step ) )

int from_who ( const TestInfo *ti, int step ) 
{
	switch (ti->algoVar[0]) {
		case 0: return GET_ID(ti) + ACTIVE_PROCS(ti, step) / 2;
		case 1:	return GET_ID(ti) + _pow2 ( step );
		default: return -1;
	}
}

int to_who ( const TestInfo *ti, int step ) 
{
	switch (ti->algoVar[0]) {
		case 0: return GET_ID(ti) - ACTIVE_PROCS(ti, step) / 2;
		case 1:	return GET_ID(ti) - _pow2 ( step );
		default: return -1;
	}
}

int do_i_receive ( const TestInfo *ti, int step )
{
	switch (ti->algoVar[0]) {
		case 0: return (GET_ID(ti) < (ACTIVE_PROCS(ti, step) / 2));
		case 1:	return (GET_ID(ti) % _pow2 ( step + 1 ) == 0);
		default: return -1;
	}
}

int do_i_send ( const TestInfo *ti, int step )
{
	switch (ti->algoVar[0]) {
		case 0: return (GET_ID(ti) >= (ACTIVE_PROCS(ti, step) / 2));
		case 1:	return (GET_ID(ti) % _pow2 ( step + 1 ) == _pow2 ( step ));
		default: return -1;
	}
}



void mergesort ( const TestInfo *ti, int *sorting )
{
	MPI_Status stat;
	const int total_size 	= GET_M ( ti );
	int size 				= GET_LOCAL_M ( ti );
	int rank 				= GET_ID ( ti );
	int active_proc 		= GET_N ( ti );
	
	int *merging = (int*) malloc ( sizeof(int) * total_size ); 

	//scattering data partitions
	if ( rank == 0 )
		_MPI_Scatter ( sorting, size, MPI_INT, MPI_IN_PLACE, size, MPI_INT, 0, MPI_COMM_WORLD );
	else
		_MPI_Scatter ( NULL, size, MPI_INT, sorting, size, MPI_INT, 0, MPI_COMM_WORLD );
		
	//sorting local partition
	qsort ( sorting, size, sizeof(int), compare );

	int step;
	for ( step = 0; step < _log2(GET_N(ti)); step++ ) {
		if ( do_i_receive( ti, step ) ) {
		
			_MPI_Recv ( (int*)sorting + size, total_size / active_proc, MPI_INT, from_who( ti, step ) , 0, MPI_COMM_WORLD, &stat );
								
			//fusion phase
			int left = 0, center = size, right = size + total_size/active_proc, k = 0;
			while ( left < size && center < right ) {
				if ( sorting[left] <= sorting[center] )
					merging[k++] = sorting[left++];
				else
					merging[k++] = sorting[center++];
			} 
			
			for ( ; left < size; left++, k++ )
				merging[k] = sorting[left]; 
			for ( ; center < right; center++, k++ )
				merging[k] = sorting[center];
			for ( left = 0; left < right; left++ )
				sorting[left] = merging[left];
					
			size += ( total_size / active_proc ); //size of the ordered sequence
			
		}
		if ( do_i_send ( ti, step ) )
			_MPI_Send ( sorting, size, MPI_INT, to_who( ti, step ), 0, MPI_COMM_WORLD );
		
		active_proc /= 2;
	}
	
	free ( merging );
}

void sort ( const TestInfo *ti )
{
	int *sorting = (int*) malloc ( sizeof(int) * GET_M ( ti )); 
	mergesort ( ti, sorting );
	free ( sorting );
}

void mainSort( const TestInfo *ti, int *sorting, long size )
{	
	mergesort ( ti, sorting );	
}

