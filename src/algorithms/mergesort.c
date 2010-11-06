
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

//from_who	: return the rank of the process (node) from which i RECEIVE data in the current step
int from_who ( int rank, int active_proc ) 
{
	return rank + active_proc / 2;	
}

//to_who	: return the rank of the process (node) from which i SEND data in the current step
int to_who ( int rank, int active_proc ) 
{
	return rank - active_proc / 2;
}

//do_i_receive 	: return a number different from 0 if the calling process (node) has to RECEIVE data FROM another process (node) in the current step.
int do_i_receive ( int rank, int active_proc )
{
	return (rank < (active_proc / 2));
}

//do_i_send	: return a number different from 0 if the calling process (node) has to SEND data TO another process (node) in the current step
int do_i_send ( int rank, int active_proc )
{
	return (rank >= (active_proc / 2));
}


//TODO: sort should return an int* to the sorted array (and even its size, of course) 
void mainSort( const TestInfo *ti, int *array, long size )
{
	MPI_Status stat;
	
	int total_size = GET_M ( ti );
	int rank = GET_ID ( ti );
	int active_proc = GET_N ( ti );
	
	int *sorting = (int*) malloc ( sizeof(int) * total_size ); //TODO..it is not necessary to allocate total_size memory for ALL nodes..
	int *merging = (int*) malloc ( sizeof(int) * total_size ); 
	
	qsort ( array, size, sizeof(int), compare );
	
	memcpy ( sorting, array, size * sizeof(int) ); //in teoria potrei farne a meno ma il codice diviene più verboso...
	//free ( array );

	int j;
	for ( j = 0; j < _log2(GET_N(ti)); ++ j ) {
		if ( do_i_receive( rank, active_proc ) )	{
		
			MPI_Recv ( (int*)sorting + size, total_size / active_proc, MPI_INT, from_who ( rank, active_proc ), 0, MPI_COMM_WORLD, &stat );
								
			//fusion phase
			int left = 0, center = size, right = size + total_size/active_proc, k = 0;
			while ( left < size && center < right ) {
				if ( sorting[left] <= sorting[center] )
					merging[k] = sorting[left++];
				else
					merging[k] = sorting[center++];
				k++;
			} 
			
			for ( ; left < size; left++, k++ )
				merging[k] = sorting[left]; 
			for ( ; center < right; center++, k++ )
				merging[k] = sorting[center];
			for ( left = 0; left < right; left++ )
				sorting[left] = merging[left];
					
			size += ( total_size / active_proc ); //size of the ordered sequence
			
		}
		if ( do_i_send( rank, active_proc ) )
			MPI_Send ( sorting, size, MPI_INT, to_who ( rank, active_proc), 0, MPI_COMM_WORLD );
		
		active_proc /= 2;
		
	}
	
}
