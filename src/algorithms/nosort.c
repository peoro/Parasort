
/**
 * @file nosort.c
 *
 * @brief This file is used just to run some tests
 * *
 * @author Paolo Giangrandi
 * @version 0.0.1
 */

#include "sorting.h"

static int _pow2( int n ) {
	int i, r = 1;
	for( i = 0; i < n; ++ i ) {
		r *= 2;
	}
	return r;
}
static int _log2( int n ) {
	int r = 0;
	for( ; n > 1; n /= 2 ) {
		r ++;
	}
	return r;
}

#define ACTIVE_PROCS(ti,step) _pow2(step)
#define GET_STEP_COUNT(ti) _log2( GET_N(ti) )

//from_who	: return the rank of the process (node) from which i RECEIVE data in the current step
int from_who( const TestInfo *ti, int step ) 
{
	return GET_ID(ti) - ( GET_N(ti) / ACTIVE_PROCS(ti,step+1) );
}

//to_who	: return the rank of the process (node) from which i SEND data in the current step
int to_who( const TestInfo *ti, int step ) 
{
	return GET_ID(ti) + ( GET_N(ti) / ACTIVE_PROCS(ti,step+1) );
}

//do_i_send	: true if the calling process (node) has to SEND data TO another process (node) in the current step
bool do_i_send( const TestInfo *ti, int step )
{
	return ! ( GET_ID(ti) % ( GET_N(ti) / ACTIVE_PROCS(ti,step) ) );
}

//do_i_receive 	: return a number different from 0 if the calling process (node) has to RECEIVE data FROM another process (node) in the current step.
bool do_i_receive( const TestInfo *ti, int step )
{
	return ! do_i_send( ti, step ) && ! ( GET_ID(ti) % ( GET_N(ti) / ACTIVE_PROCS(ti,step+1) ) ) ;
}

void mainSort( const TestInfo *ti, int *array, long size )
{
	(void) array;
	(void) size;
	
	// printf( "Should sort an array of %ld elements...\n", size );
	
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
}
void sort( const TestInfo *ti )
{
	// printf( "Should sort an array...\n" );
	mainSort( ti, NULL, 0 );
}
