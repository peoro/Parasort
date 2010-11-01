
/* compile with
   export ALGO=nosort && mpicc -g -I.. -L.. -L/usr/lib/openmpi -lmpi -shared -O2 -Wl,-soname,$ALGO -o lib$ALGO.so $ALGO.c && cp lib$ALGO.so ~/.spd/algorithms/
 */

#include "sorting.h"

void sort( const TestInfo *ti, int *array, long size )
{
	printf( "Should sort an array of %ld elements...\n", size );
}
