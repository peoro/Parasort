
/**
 * @file nosort.c
 *
 * @brief This file is used just to run some tests
 * *
 * @author Paolo Giangrandi
 * @version 0.0.1
 */

#include "sorting.h"
#include "utils.h"
#include "math.h"

//#define KMERGE
#define MERGE
//#define QSORT

#define GET_STEP_COUNT(ti) _log2( GET_N(ti) )

#if defined(QSORT)

#define ACTIVE_PROCS(ti,step) _pow2(step)

int from_who( const TestInfo *ti, int step ) 
{
	return GET_ID(ti) - ( GET_N(ti) / ACTIVE_PROCS(ti,step+1) );
}

int to_who( const TestInfo *ti, int step ) 
{
	return GET_ID(ti) + ( GET_N(ti) / ACTIVE_PROCS(ti,step+1) );
}

bool do_i_send( const TestInfo *ti, int step )
{
	return ! ( GET_ID(ti) % ( GET_N(ti) / ACTIVE_PROCS(ti,step) ) );
}

bool do_i_receive( const TestInfo *ti, int step )
{
	return ! do_i_send( ti, step ) && ! ( GET_ID(ti) % ( GET_N(ti) / ACTIVE_PROCS(ti,step+1) ) ) ;
}

#elif defined(KMERGE)
//TODO: qua definisco solo l'alternative mapping del k-way merge..

#define GET_K(ti) ti->algoVar[0]

int K_WAY_SENDS ( int n, int k ) {
	int i = 0, sends = 0;
	for ( ; i < _logk(n,k); i++ )
		sends += pow ( k, i );
	return (k-1)*sends;
}

int from_who( const TestInfo *ti, int step ) 
{
	return 0;
}

int to_who( const TestInfo *ti, int step ) 
{
	return GET_ID(ti) - ( _powK( GET_K(ti), step) * (rank % GET_K(ti));
}

bool do_i_send( const TestInfo *ti, int step )
{
	return true;
}

bool do_i_receive( const TestInfo *ti, int step )
{
	return true;
}

#elif defined(MERGE)

//mapping 0: standard mapping
//mapping 1: first steps communications made between adjacent processors 
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

#endif


void mainSort( const TestInfo *ti, int *array, long size )
{
	(void) array;
	(void) size;
	
	// printf( "Should sort an array of %ld elements...\n", size );
	
#if defined (MERGE)	
	// process with rank algoVar[0] will print its stencil...
	int step = 0;
	int mapping = ti->algoVar[1];
	if( GET_ID(ti) == ti->algoVar[0] ) {
		for( step = 0; step < GET_STEP_COUNT(ti); ++ step ) {
			if( do_i_send( ti, step ) ) {
				printf( "%d @ step %d :: sending to %d\n", GET_ID(ti), step, to_who( ti, step ) );
			}
			if( do_i_receive( ti, step ) ) {
				printf( "%d @ step %d :: recveiving from %d\n", GET_ID(ti), step, from_who( ti, step ) );
			}
			if( ! do_i_send( ti, step ) && ! do_i_receive( ti, step ) ) {
				printf( "%d @ step %d :: doing nothing\n", GET_ID(ti), step );
			}
		}
	}

#elif defined (KMERGE)
	if ( GET_ID(ti) == 0 )	
		printf("this %i-way mergesort through %i processors will make %i sends\n", ti->algoVar[1], GET_N(ti), K_WAY_SENDS ( GET_N(ti), ti->algoVar[1] ));

	
#elif defined (QSORT)
	// process with rank algoVar[0] will print its stencil...
	int step = 0;
	if( GET_ID(ti) == ti->algoVar[0] ) {
		for( step = 0; step < GET_STEP_COUNT(ti); ++ step ) {
			if( do_i_send( ti, step ) ) {
				printf( "%d @ step %d :: sending to %d\n", GET_ID(ti), step, to_who( ti, step ) );
			}
			if( do_i_receive( ti, step ) ) {
				printf( "%d @ step %d :: recveiving from %d\n", GET_ID(ti), step, from_who( ti, step ) );
			}
			if( ! do_i_send( ti, step ) && ! do_i_receive( ti, step ) ) {
				printf( "%d @ step %d :: doing nothing\n", GET_ID(ti), step );
			}
		}
	}
	
#endif

}
void sort( const TestInfo *ti )
{
	// printf( "Should sort an array...\n" );
	mainSort( ti, NULL, 0 );
}
