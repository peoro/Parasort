#include "../sorting.h"
#include "../common.h"


void mainSort( const TestInfo *ti, Data *data )
{
	DAL_send( data, 1 );
	DAL_receive( data, GET_M(ti), 1 );
}


void sort( const TestInfo *ti )
{
	if( GET_ID(ti) == 1 ) {
		Data data;
		DAL_receive( &data, GET_M(ti), 0 );
		sequentialSort( ti, &data );
		DAL_send( &data, 1 );
		DAL_destroy( &data );
	}
}
