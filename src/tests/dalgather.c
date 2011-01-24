
// just testing DAL_gather primitive

#include "tests.h"
#include "../dal.h"
#include "../dal_internals.h"

#define MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define MAX(a,b) ( (a)>(b) ? (a) : (b) )

void TEST_DAL_gatherSend( Data *data, int root )
{
	int i, j;

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	//VALID ONLY FOR THIS TEST//
	globalBuf.array.size = GET_N();
	////////////////////////////

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	dal_size_t count = DAL_dataSize(data);
	int num_iterations = count / blockSize + (count % blockSize > 0);
	int sendCount, tmp;


	switch( data->medium ) {
		case File: {

			for ( i=0; i<num_iterations; i++ ) {
				tmp = MIN( blockSize, count-i*blockSize );
				sendCount = tmp > 0 ? tmp : 0;
				DAL_dataCopyOS( data, i*blockSize, &globalBuf, 0, sendCount );

				MPI_Gatherv( globalBuf.array.data, sendCount, MPI_INT, NULL, NULL, NULL, MPI_INT, root, MPI_COMM_WORLD );
			}
			break;
		}
		case Array: {
			int sendDispl = 0;

			for ( i=0; i<num_iterations; i++ ) {
				tmp = MIN( blockSize, count-i*blockSize );
				sendCount = tmp > 0 ? tmp : 0;
				MPI_Gatherv( data->array.data+sendDispl, sendCount, MPI_INT, NULL, NULL, NULL, MPI_INT, root, MPI_COMM_WORLD );
				sendDispl += sendCount;
			}
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}

	DAL_releaseGlobalBuffer( &globalBuf );
}
void TEST_DAL_gatherReceive( Data *data, dal_size_t size )
{
	SPD_ASSERT( size % GET_N() == 0, "size should be a multiple of %d (but it's "DST")", GET_N(), size );
	int rc[GET_N()];
	int rd[GET_N()];
	int i, j;

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	DAL_ASSERT( globalBuf.array.size >= GET_N(), &globalBuf, "The global-buffer is too small for a gather communication (its size is "DST", but there are %d processes)", globalBuf.array.size, GET_N() );

	//VALID ONLY FOR THIS TEST//
	globalBuf.array.size = GET_N();
	////////////////////////////

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	dal_size_t count = size/GET_N();
	int num_iterations = count / blockSize + (count % blockSize > 0);
	int r, tmp;

	//Data to store the received elements
	Data recvData;
	DAL_init( &recvData );
	SPD_ASSERT( DAL_allocData(&recvData, size), "not enough space to allocate data" );

	if( data->medium == File || recvData.medium == File ) {
		for ( i=0; i<num_iterations; i++ ) {
			tmp = MIN( blockSize, (count-i*blockSize) );

			for ( r=0, j=0; j<GET_N(); j++ ) {
				rc[j] =	tmp > 0 ? tmp : 0;	//Number of elements to be sent to process j by MPI_Alltoallv
				rd[j] = r;
				r += rc[j];
			}
			DAL_dataCopyOS( data, i*blockSize, &globalBuf, rd[GET_ID()], rc[GET_ID()] );

			MPI_Gatherv( MPI_IN_PLACE, rc[GET_ID()], MPI_INT, globalBuf.array.data, rc, rd, MPI_INT, GET_ID(), MPI_COMM_WORLD );

			for ( j=0; j<GET_N(); j++ )
				DAL_dataCopyOS( &globalBuf, rd[j], &recvData, j*count + i*blockSize, rc[j] );
		}
	}
	else if (data->medium == Array && recvData.medium == Array ) {

		for ( i=0; i<num_iterations; i++ ) {
			tmp = MIN( blockSize, (count-i*blockSize) );

			for ( j=0; j<GET_N(); j++ ) {
				rc[j] =	tmp > 0 ? tmp : 0;	//Number of elements to be sent to process j by MPI_Alltoallv
				rd[j] = j*count + i*blockSize;
			}
			MPI_Gatherv( data->array.data+rd[GET_ID()], rc[GET_ID()], MPI_INT, recvData.array.data, rc, rd, MPI_INT, GET_ID(), MPI_COMM_WORLD );
		}
	}
	else
		DAL_UNSUPPORTED( data );

	DAL_destroy( data );
	*data = recvData;

	DAL_releaseGlobalBuffer( &globalBuf );
}
/**
* @brief Gathers data from all processes
*
* @param[in,out] 	data  		Data to be gathered/sent
* @param[in] 		size     	Number of elements to be gathered from each process
* @param[in] 		root     	Rank of the root process
*/
void TEST_DAL_gather( Data *data, dal_size_t count, int root )
{
	if( GET_ID() == root ) {
		return TEST_DAL_gatherReceive( data, count*GET_N() );
	}
	else {
		return TEST_DAL_gatherSend( data, root );
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

	dal_size_t count = 3;
	int size = n*count;	//size of data to collect
	int r;

	//Data
	Data d;
	DAL_init( &d );

	if ( GET_ID() % 2 ) {
		SPD_ASSERT( DAL_allocArray( &d, count ), "error allocating data..." );
	}
	else
		SPD_ASSERT( DAL_allocData( &d, count ), "error allocating data..." );

	//tmp buffer to init data
	Data buffer;
	DAL_init( &buffer );
	SPD_ASSERT( DAL_allocArray( &buffer, count ), "error allocating buffer..." );
	for( i=0; i<count; i++ )
		buffer.array.data[i] = GET_ID();

	//Initializing data
	DAL_dataCopy( &buffer, &d );

	//Destroying tmp buffer
	DAL_destroy( &buffer );

	DAL_PRINT_DATA( &d, "This is what I had" );

	//Gather communication
	TEST_DAL_gather( &d, count, root );

	if ( GET_ID() == root ) {
		DAL_PRINT_DATA( &d, "This is what I got" );
	}

	DAL_destroy( &d );

	DAL_finalize( );
	return 0;
}

