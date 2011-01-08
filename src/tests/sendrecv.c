
// just testing send and receive

#include "tests.h"

int main( int argc, char **argv )
{
	int i, n;
	int var;
	
	MPI_Init( &argc, &argv );
	
	n = GET_N();
	if( n < 2 ) {
		TESTS_ERROR( 1, "Use this with at least 2 processes!" );
	}
	
	if( GET_ID() == 0 ) {
		var = 666;
		TESTS_MPI_SEND( &var, 1, MPI_INT, GET_ID()+1 );
	}
	else if( GET_ID() < GET_N()-1 ) {
		TESTS_MPI_RECEIVE( &var, 1, MPI_INT, GET_ID()-1 );
		TESTS_MPI_SEND( &var, 1, MPI_INT, GET_ID()+1 );
	}
	else /* GET_ID() == GET_N()-1 */ {
		TESTS_MPI_RECEIVE( &var, 1, MPI_INT, GET_ID()-1 );
	}
	
	SPD_DEBUG( "Got %d", var );
	
	MPI_Finalize( );
	return 0;
}
	

