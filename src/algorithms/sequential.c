#include "../sorting.h"
#include "../utils.h"

void mainSort( const TestInfo *ti, int *data, long size )
{
	if ( GET_N( ti ) > 1 ) {
		printf ( "\nError: number of processes greater than 1!\n\n" );
		return;
	}
	qsort( data, GET_M( ti ), sizeof(int), compare );
}

void sort( const TestInfo *ti )
{
}
