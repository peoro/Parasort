
// just testing send and receive

#include "tests.h"
#include <stdlib.h>

int main( int argc, char **argv )
{
	char *mem, *mem_recv;;
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
	mem_recv = (char*) malloc( size );
	if( ! mem_recv ) {
		TESTS_ERROR( 1, "malloc(%ld) failed", size );
	}
	
	
	long blockSize = size / count;
	long block_disp = ( blockSize / GET_N() ); //only for scatter and alltoall
	long current = 0;
	Time start, end;
	TimeDiff res;
	
	// MPI_SEND
	{
		if( GET_ID() == 0 ) {
			//SPD_DEBUG( "MPI_SEND: %ld blocks of %ld bytes", count, blockSize );
			start = now( );
			for( i = 0; i < count; ++ i ) {
				TESTS_MPI_SEND( mem + current, blockSize, MPI_CHAR, 1 );
				current += blockSize;
			}
			end = now( );
			res = timeDiff( start, end );
			//SPD_DEBUG( "%ld.%.3ld secs (%7ld usecs) to send", res.time, res.mtime, res.utime );
			//SPD_DEBUG( "%ld.%.3ld", res.time, res.mtime );
			printf ( "%ld.%.3ld\t", res.time, res.mtime );
		}
		else if( GET_ID() == 1 ) {
			//SPD_DEBUG( "Receiving %ld blocks of %ld bytes", count, blockSize );
			start = now( );
			for( i = 0; i < count; ++ i ) {
				TESTS_MPI_RECEIVE( mem + current, blockSize, MPI_CHAR, 0 );
				current += blockSize;
			}
			end = now( );
			res = timeDiff( start, end );
			//SPD_DEBUG( "%ld.%.3ld secs (%7ld usecs) to receive", res.time, res.mtime, res.utime );
		}
	}
	
	
	//MPI_SCATTER
	{	
		//if ( GET_ID() == 0 )
			//SPD_DEBUG( "MPI_SCATTER: %ld blocks of %ld bytes, %ld bytes to each process at a time", count, blockSize, block_disp );
		
		SPD_ASSERT ( blockSize % GET_N() == 0, "Scattered block must be of equal size, now blockSize mod N = %ld", blockSize % GET_N() );
		
		current = 0;
		
		start = now( );
		for( i = 0; i < count; ++ i ) {
			TESTS_MPI_SCATTER( mem + current, block_disp, MPI_CHAR, mem_recv + i*block_disp, 0 );
			current += blockSize;
		}
		end = now( );
		res = timeDiff( start, end );
		
		if ( GET_ID() == 0 )
			printf ( "%ld.%.3ld\t", res.time, res.mtime );
	}
	
	//MPI_ALLTOALL
	{
		//if ( GET_ID() == 0 )
			//SPD_DEBUG( "MPI_ALLTOALL: %ld blocks of %ld bytes, %ld bytes to each process at a time", count, blockSize, block_disp );
		
		SPD_ASSERT ( blockSize % GET_N() == 0, "Scattered block must be of equal size, now blockSize mod N = %ld", blockSize % GET_N() );
		
		current = 0;
		
		start = now( );
		for( i = 0; i < count; ++ i ) {
			TESTS_MPI_ALLTOALL( mem + current, block_disp, MPI_CHAR, mem_recv + current );
			current += blockSize;
		}
		end = now( );
		res = timeDiff( start, end );
		
		if ( GET_ID() == 0 )
			printf ( "%ld.%.3ld\n", res.time, res.mtime );
	}
	
	free ( mem );
	free ( mem_recv );
	
	MPI_Finalize( );
	return 0;
}
	

