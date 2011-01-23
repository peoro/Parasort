
// just testing DAL_scatter primitive

#include "tests.h"
#include "../dal.h"
#include "../dal_internals.h"

#define MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define MAX(a,b) ( (a)>(b) ? (a) : (b) )


void TEST_DAL_scatterSend( Data *data )
{
	int i, j;

	SPD_ASSERT( DAL_dataSize(data) % GET_N() == 0, "Data size should be a multiple of n=%d, but it's "DST, GET_N(), DAL_dataSize(data) );

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	DAL_ASSERT( globalBuf.array.size >= GET_N(), &globalBuf, "The global-buffer is too small for a scatter communication (its size is "DST", but there are %d processes)", globalBuf.array.size, GET_N() );

	//VALID ONLY FOR THIS TEST//
	globalBuf.array.size = GET_N();
	////////////////////////////

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	dal_size_t count = DAL_dataSize(data) / GET_N();
	int num_iterations = count / blockSize + (count % blockSize > 0);
	int tmp;

	switch( data->medium ) {
		case File: {

			for ( i=0; i<num_iterations; i++ ) {
				tmp = MIN( blockSize, (count-i*blockSize) );

				for ( j=0; j<GET_N(); j++ )
					DAL_dataCopyOS( data, j*count + i*blockSize, &globalBuf, j*blockSize, tmp );

				MPI_Scatter( globalBuf.array.data, tmp, MPI_INT, MPI_IN_PLACE, tmp, MPI_INT, GET_ID(), MPI_COMM_WORLD );
			}

			//TODO: resize root data (maybe a DAL_reallocData function would be useful)
			data->file.size = count;
			break;
		}
		case Array: {
			int sc[GET_N()], sd[GET_N()];

			for ( i=0; i<num_iterations; i++ ) {

				for ( j=0; j<GET_N(); j++ ) {
					tmp = MIN( blockSize, (count-i*blockSize) );
					sc[j] =	tmp > 0 ? tmp : 0;	//Number of elements to be sent to process j by MPI_Alltoallv
					sd[j] = j*count + i*blockSize;
				}
				MPI_Scatterv( data->array.data, sc, sd, MPI_INT, MPI_IN_PLACE, sc[GET_ID()], MPI_INT, GET_ID(), MPI_COMM_WORLD );
			}
			SPD_ASSERT( DAL_reallocArray( data, count ), "not enough memory to allocate data" );
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}

	DAL_releaseGlobalBuffer( &globalBuf );
}
void TEST_DAL_scatterReceive( Data *data, dal_size_t count, int root )
{
	int i, j;

// 	SPD_ASSERT( DAL_allocArray( data, count ), "not enough memory to allocate data" );
	SPD_ASSERT( DAL_allocData( data, count ), "not enough space to allocate data" );

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	//VALID ONLY FOR THIS TEST//
	globalBuf.array.size = GET_N();
	////////////////////////////

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	int num_iterations = count / blockSize + (count % blockSize > 0);
	int tmp;

	switch( data->medium ) {
		case File: {

			for ( i=0; i<num_iterations; i++ ) {
				tmp = MIN( blockSize, count-i*blockSize );
				MPI_Scatter( NULL, 0, MPI_INT, globalBuf.array.data, tmp, MPI_INT, root, MPI_COMM_WORLD );

				DAL_dataCopyOS( &globalBuf, 0, data, i*blockSize, tmp );
			}
			break;
		}
		case Array: {
			int recvDispl = 0;

			for ( i=0; i<num_iterations; i++ ) {
				tmp = MIN( blockSize, count-i*blockSize );
				MPI_Scatterv( NULL, NULL, NULL, MPI_INT, data->array.data+recvDispl, tmp, MPI_INT, root, MPI_COMM_WORLD );
				recvDispl += tmp;
			}
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}

	DAL_releaseGlobalBuffer( &globalBuf );
}
/**
* @brief Scatters data among all processes
*
* @param[in] data  		Data to be scattered
* @param[in] count     	Number of elements per process
* @param[in] root     	Rank of the root process
*/
void TEST_DAL_scatter( Data *data, dal_size_t count, int root )
{
	if( GET_ID() == root ) {
		return TEST_DAL_scatterSend( data );
	}
	else {
		return TEST_DAL_scatterReceive( data, count, root );
	}
}

int main( int argc, char **argv )
{
	int i, n;
	const int root = 0;

	DAL_initialize( &argc, &argv );

	n = GET_N();
	if( n < 2 ) {
		TESTS_ERROR( 1, "Use this with at least 2 processes!" );
	}


	int size = n*2;	//size of data to collect

	//Data
	Data d;
	DAL_init( &d );

	if ( GET_ID() == root ) {
// 		SPD_ASSERT( DAL_allocArray( &d, size ), "error allocating data..." );
		SPD_ASSERT( DAL_allocData( &d, size ), "error allocating data..." );

		//tmp buffer to init data
		Data buffer;
		DAL_init( &buffer );
		SPD_ASSERT( DAL_allocArray( &buffer, size ), "error allocating buffer..." );
		for( i=0; i<size; i++ )
			buffer.array.data[i] = i;

		//Initializing data
		DAL_dataCopy( &buffer, &d );

		//Destroying tmp buffer
		DAL_destroy( &buffer );

	}

	if( GET_ID() == root )
		DAL_PRINT_DATA( &d, "This is what I had" );

	//Scatter communication
	TEST_DAL_scatter( &d, size/n, root );

	DAL_PRINT_DATA( &d, "This is what I got" );

	DAL_destroy( &d );

	DAL_finalize( );
	return 0;
}

