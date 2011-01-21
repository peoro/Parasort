
// just testing send and receive

#include "tests.h"
#include "../dal.h"
#include "../dal_internals.h"

void testCopy( dal_size_t size ) {
	Data f1;
	DAL_init( &f1 );
	DAL_allocFile( &f1, size );
	Data f2;
	DAL_init( &f2 );
	DAL_allocFile( &f2, size );

	Data a1;
	DAL_init( &a1 );
	DAL_allocBuffer( &a1, size );
	Data a2;
	DAL_init( &a2 );
	DAL_allocBuffer( &a2, size );

	dal_size_t i;

	if( size > DAL_dataSize(&a1) ) {
		for( i=0; i<size; i++ ) {
			int x = rand();
			fwrite( &x, sizeof(int), 1, f1.file.handle );
		}
		DAL_dataCopy( &f1, &f2 );

		rewind( f1.file.handle );
		rewind( f2.file.handle );

		for( i=0; i<size; i++ ) {
			int x1, x2;
			fread( &x1, sizeof(int), 1, f1.file.handle );
			fread( &x2, sizeof(int), 1, f2.file.handle );
			SPD_ASSERT( x1 == x2, "Something went wrong" );
		}
	}
	else {
		for( i=0; i<size; i++ )
			a2.array.data[i] = rand();
		DAL_dataCopy( &a2, &a1 );
		DAL_dataCopy( &a1, &f1 );
		DAL_dataCopy( &f1, &f2 );
		DAL_dataCopy( &f2, &a2 );

		for( i=0; i<size; i++ )
			SPD_ASSERT( a1.array.data[i] == a2.array.data[i], "Something went wrong" );
	}
	DAL_destroy( &a1 );
	DAL_destroy( &a2 );
	DAL_destroy( &f1 );
	DAL_destroy( &f2 );
}

int main( int argc, char **argv )
{
	DAL_initialize( &argc, &argv );

	//DAL_dataCopy TEST
	dal_size_t size;
	SPD_DEBUG( "Testing DAL_dataCopy with size from 1 to 1024 " );
	for( size = 1; size < 1024; size++ ) {
		testCopy( size );
	}
	SPD_DEBUG( "Testing DAL_dataCopy with size "DST, DAL_allowedBufSize( ) );
	testCopy( DAL_allowedBufSize( ) );
	SPD_DEBUG( "Testing DAL_dataCopy with size "DST, DAL_allowedBufSize( )+1 );
	testCopy( DAL_allowedBufSize( )+1 );


	DAL_finalize( );
	return 0;
}


