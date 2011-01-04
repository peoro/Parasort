
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

void mergesort ( const TestInfo *ti, Data *data_local )
{
	const int 	total_size 	= GET_M ( ti );
	const int 	rank 		= GET_ID ( ti );
	int 		active_proc = GET_N ( ti );
	int 		*merging 	= (int*) malloc ( sizeof(int) * total_size ); /*TODO: needs to be limited somehow to the memory size*/

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

			//Enlarging data_local to allow it to contain the new partition of data
			int old_size = data_local->array.size;
			DAL_reallocArray ( data_local, data_local->array.size + data_received.array.size );

			//memory merging phase
			int left_a = 0, left_b = 0, k = 0;
			while ( left_a < old_size && left_b < data_received.array.size ) {
				if ( data_local->array.data[left_a] <= data_received.array.data[left_b] )
					merging[k++] = data_local->array.data[left_a++];
				else
					merging[k++] = data_received.array.data[left_b++];
				}

			for ( ; left_a < old_size; left_a++, k++ )
				merging[k] = data_local->array.data[left_a];
			for ( ; left_b < data_received.array.size; left_b++, k++ )
				merging[k] = data_received.array.data[left_b];
			for ( left_a = 0; left_a < data_local->array.size; left_a++ )
				data_local->array.data[left_a] = merging[left_a];

			//free memory
			DAL_destroy ( &data_received );
		}
		if ( do_i_send ( ti, step ) )
			DAL_send ( data_local, to_who( ti, step ) );

		//keeps updated the number of processes that partecipate to the sorting in the next step
		active_proc /= 2;
	}

	stopPhase( ti, localP );

	free ( merging );
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
}
