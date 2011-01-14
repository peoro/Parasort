
// just testing send and receive

#include "tests.h"
#include "../dal.h"
#include "../dal_internals.h"

int main( int argc, char **argv )
{
	int i, n;
	Data d;
	DAL_init( &d );
	
	DAL_initialize( &argc, &argv );
	
	n = GET_N();
	if( n < 2 ) {
		TESTS_ERROR( 1, "Use this with at least 2 processes!" );
	}
	
	if( GET_ID() == 0 ) {
		Data tmp;
		DAL_init( &tmp );
		SPD_ASSERT( DAL_allocArray( &tmp, 1 ), "error allocating array..." );
		tmp.array.data[0] = 666;
		
		SPD_ASSERT( DAL_allocData( &d, 1 ), "error allocating data..." );
		
		DAL_dataCopy( &tmp, &d );
		
		DAL_destroy( &tmp );
		
		DAL_send( &d, GET_ID()+1 );
	}
	else if( GET_ID() < GET_N()-1 ) {
		DAL_receive( &d, 1, GET_ID()-1 );
		DAL_send( &d, GET_ID()+1 );
	}
	else /* GET_ID() == GET_N()-1 */ {
		DAL_receive( &d, 1, GET_ID()-1 );
	}
	
	DAL_PRINT_DATA( &d, "This is what I got" );
	
	DAL_destroy( &d );
	
	DAL_finalize( );
	return 0;
}
	

