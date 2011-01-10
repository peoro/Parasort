
// just testing send and receive

#include "tests.h"
#include <stdlib.h>

int main( int argc, char **argv )
{
	char *mem;
	long size, count;
	long i;
	
	MPI_Init( &argc, &argv );
	
	if( argc != 3 ) {
		TESTS_ERROR( 1, "Usage: %s total_memory num_sends", argv[0] );
	}
	size = atoi( argv[1] );
	count = atoi( argv[2] );
	if( size <= 0 ) {
		TESTS_ERROR( 1, "total_memory must be positive" );
	}
	if( count <= 0 || size%count != 0 ) {
		TESTS_ERROR( 1, "num_sends must be positive and integer divisor of size" );
	}
	
	if( GET_N() < 2 ) {
		TESTS_ERROR( 1, "you must use 2 processes" );
	}
	
	// broadcasting size and count
	if( GET_ID() == 0 ) {
		TESTS_MPI_SEND( &size, 1, MPI_LONG, 1 );
		TESTS_MPI_SEND( &count, 1, MPI_LONG, 1 );
	}
	else if( GET_ID() == 1 ) {
		TESTS_MPI_RECEIVE( &size, 1, MPI_LONG, 0 );
		TESTS_MPI_RECEIVE( &count, 1, MPI_LONG, 0 );
	}
	
	// allocating memory
	mem = (char*) malloc( size );
	if( ! mem ) {
		TESTS_ERROR( 1, "malloc(%ld) failed", size );
	}
	
	// sending data
	{
		long blockSize = size / count;
		long current = 0;
		Time start, end;
		TimeDiff res;
		if( GET_ID() == 0 ) {
			SPD_DEBUG( "Sending %ld blocks of %ld bytes", count, blockSize );
			start = now( );
			for( i = 0; i < count; ++ i ) {
				TESTS_MPI_SEND( mem + current, blockSize, MPI_CHAR, 1 );
				current += blockSize;
			}
			end = now( );
			res = timeDiff( start, end );
			SPD_DEBUG( "%ld.%.3ld secs (%7ld usecs) to send", res.time, res.mtime, res.utime );
		}
		else if( GET_ID() == 1 ) {
			SPD_DEBUG( "Receiving %ld blocks of %ld bytes", count, blockSize );
			start = now( );
			for( i = 0; i < count; ++ i ) {
				TESTS_MPI_RECEIVE( mem + current, blockSize, MPI_CHAR, 0 );
				current += blockSize;
			}
			end = now( );
			res = timeDiff( start, end );
			SPD_DEBUG( "%ld.%.3ld secs (%7ld usecs) to receive", res.time, res.mtime, res.utime );
		}
	}
	
	MPI_Finalize( );
	return 0;
}
	

