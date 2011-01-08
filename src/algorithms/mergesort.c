
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

/**************************************/
/*               STENCIL              */
/**************************************/

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
		case 0: return (GET_ID(ti) >= (ACTIVE_PROCS(ti, step) / 2) && GET_ID(ti) < ACTIVE_PROCS(ti, step));
		case 1:	return (GET_ID(ti) % _pow2 ( step + 1 ) == _pow2 ( step ));
		default: return -1;
	}
}



/**************************************/
/*            ALGORITHMIC             */
/**************************************/

//merges two data on a File
void fileFusion ( Data *data_local, Data *data_received, Data *data_merged ) 
{
	//data_merged has to be initialized within this function (medium -> File, file name..)
	DAL_UNIMPLEMENTED ( data_local );
}

//merges two data in memory. data_merged is preallocated 
void memoryFusion ( Data *data_local, Data *data_received, Data *data_merged ) 
{
	int left_a = 0, left_b = 0, k = 0;
	while ( left_a < data_local->array.size && left_b < data_received->array.size ) {
		if ( data_local->array.data[left_a] <= data_received->array.data[left_b] )
			data_merged->array.data[k++] = data_local->array.data[left_a++];
		else
			data_merged->array.data[k++] = data_received->array.data[left_b++];
	}

	for ( ; left_a < data_local->array.size; left_a++, k++ )
		data_merged->array.data[k] = data_local->array.data[left_a];
	for ( ; left_b < data_received->array.size; left_b++, k++ )
		data_merged->array.data[k] = data_received->array.data[left_b];
}

//postcondition: data_local and data_received will not be valid anymore after a call to this function
Data fusion ( Data *data_local, Data *data_received )
{
	Data merging;
	int merging_size = DAL_dataSize ( data_local ) + DAL_dataSize ( data_received );
	
	DAL_init ( &merging );
	
	switch ( data_local->medium ) {
		case File: {
			fileFusion ( data_local, data_received, &merging );
			break;
		}
		case Array: {
			if ( DAL_allocArray ( &merging, merging_size )) 
				//memory merge
				memoryFusion ( data_local, data_received, &merging );
			else 
				fileFusion ( data_local, data_received, &merging );
			break;
		}
		default: 
			DAL_UNSUPPORTED( data_local );
	}
	
	DAL_destroy ( data_local );	
	DAL_destroy ( data_received );
	
	return merging;
}


void mergesort ( const TestInfo *ti, Data *data_local )
{
	const int 	total_size 	= GET_M ( ti );
	const int 	rank 		= GET_ID ( ti );
	int 		active_proc = GET_N ( ti );

	Data 		data_received;
	PhaseHandle	scatterP, localP;

	DAL_init ( &data_received );

	//scattering data partitions
	scatterP = startPhase( ti, "Scattering" );
	DAL_scatter ( data_local, GET_LOCAL_M( ti ), 0 );
	stopPhase( ti, scatterP );

	//sorting
	localP = startPhase( ti, "Sorting" );
	sequentialSort ( ti, data_local );

	int step;
	for ( step = 0; step < _log2(GET_N(ti)); step++ ) {
		if ( do_i_receive( ti, step ) ) {
			DAL_receive ( &data_received, total_size / active_proc, from_who( ti, step ) );
			
			//merge phase
			*data_local = fusion ( data_local, &data_received );
		}
		if ( do_i_send ( ti, step ) )
			DAL_send ( data_local, to_who( ti, step ) );

		//keeps updated the number of processes that partecipate to the sorting in the next step
		active_proc /= 2;
	}

	if ( ! DAL_isInitialized( &data_received ))
		DAL_destroy ( &data_received );
		
	stopPhase( ti, localP );
}

void sort ( const TestInfo *ti )
{
	Data data_local;
	DAL_init ( &data_local );
	
	mergesort ( ti, &data_local );
	
	DAL_destroy ( &data_local );
}

void mainSort( const TestInfo *ti, Data *data )
{
	mergesort ( ti, data );
	
	DAL_ASSERT( DAL_dataSize(data) == GET_M(ti), data, "data isn't as big as it was originally..." );
}
