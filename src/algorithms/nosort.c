
/* compile with
   export ALGO=nosort && mpicc -g -I.. -L.. -L/usr/lib/openmpi -lmpi -shared -O2 -Wl,-soname,$ALGO -o lib$ALGO.so $ALGO.c && cp lib$ALGO.so ~/.spd/algorithms/
 */

#include "sorting.h"

void mainSort( const TestInfo *ti, int *array, long size )
{
	// printf( "Should sort an array of %ld elements...\n", size );
	
	int step = 0;
	
	if( GET_ID(ti) == ti->algoVar[0] ) {
		for( step = 0; step < GET_STEP_COUNT(ti); ++ step ) {
			if( do_i_receive( ti, step ) ) {
				printf( "%d @ step %d :: recveiving from %d\n", from_who( ti, step ) );
			}
			else if( do_i_send( ti, step ) ) {
				printf( "%d @ step %d :: sending to %d\n", to_who( ti, step ) );
			}
		}
	}
}
void sort( const TestInfo *ti )
{
	// printf( "Should sort an array...\n" );
	mainSort( ti, NULL, 0 );
}
