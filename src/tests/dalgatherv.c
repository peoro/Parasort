
// just testing DAL_gather primitive

#include "tests.h"
#include "../dal.h"
#include "../dal_internals.h"

#define MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define MAX(a,b) ( (a)>(b) ? (a) : (b) )

void TEST_DAL_gathervSend( Data *data, int root )
{
	int i, j;

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	//VALID ONLY FOR THIS TEST//
	globalBuf.array.size = GET_N();
	////////////////////////////

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	long max_count;
	long size = DAL_dataSize(data);
	MPI_Allreduce( &size, &max_count, 1, MPI_LONG, MPI_MAX, MPI_COMM_WORLD );
	int num_iterations = max_count / blockSize + max_count % blockSize;
	int sendCount, tmp;


	switch( data->medium ) {
		case File: {

			for ( i=0; i<num_iterations; i++ ) {
				tmp = MIN( blockSize, size-i*blockSize );
				sendCount = tmp > 0 ? tmp : 0;

				if ( sendCount )
					DAL_dataCopyOS( data, i*blockSize, &globalBuf, 0, sendCount );

				MPI_Gatherv( globalBuf.array.data, sendCount, MPI_INT, NULL, NULL, NULL, MPI_INT, root, MPI_COMM_WORLD );
			}
			break;
		}
		case Array: {
			int sendDispl = 0;

			for ( i=0; i<num_iterations; i++ ) {
				tmp = MIN( blockSize, size-i*blockSize );
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
void TEST_DAL_gathervReceive( Data *data, long *counts, long *displs )
{
	int rc[GET_N()];
	int rd[GET_N()];
	int i, j;

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	DAL_ASSERT( globalBuf.array.size >= GET_N(), &globalBuf, "The global-buffer is too small for a gather communication (its size is %ld, but there are %d processes)", globalBuf.array.size, GET_N() );

	//VALID ONLY FOR THIS TEST//
	globalBuf.array.size = GET_N();
	////////////////////////////

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	long max_count = 0;
	MPI_Allreduce( &counts[GET_ID()], &max_count, 1, MPI_LONG, MPI_MAX, MPI_COMM_WORLD );
	int num_iterations = max_count / blockSize + max_count % blockSize;
	int r, tmp;

	//Computing the total number of elements to be received
	long rcount = 0;
	for ( i=0; i<GET_N(); i++ ) {
		rcount += counts[i];
	}

	//Data to store the received elements
	Data recvData;
	DAL_init( &recvData );
	SPD_ASSERT( DAL_allocData(&recvData, rcount), "not enough space to allocate data" );

	if( data->medium == File || recvData.medium == File ) {
		for ( i=0; i<num_iterations; i++ ) {

			for ( r=0, j=0; j<GET_N(); j++ ) {
				tmp = MIN( blockSize, (counts[j]-i*blockSize) );
				rc[j] =	tmp > 0 ? tmp : 0;	//Number of elements to be sent to process j by MPI_Alltoallv
				rd[j] = r;
				r += rc[j];
			}
			if ( rc[GET_ID()] )
				DAL_dataCopyOS( data, i*blockSize, &globalBuf, rd[GET_ID()], rc[GET_ID()] );

			MPI_Gatherv( MPI_IN_PLACE, rc[GET_ID()], MPI_INT, globalBuf.array.data, rc, rd, MPI_INT, GET_ID(), MPI_COMM_WORLD );

			for ( j=0; j<GET_N(); j++ )
				if( rc[j] )
					DAL_dataCopyOS( &globalBuf, rd[j], &recvData, displs[j] + i*blockSize, rc[j] );
		}
	}
	else if (data->medium == Array && recvData.medium == Array ) {

		for ( i=0; i<num_iterations; i++ ) {

			for ( j=0; j<GET_N(); j++ ) {
				tmp = MIN( blockSize, (counts[j]-i*blockSize) );
				rc[j] =	tmp > 0 ? tmp : 0;	//Number of elements to be sent to process j by MPI_Alltoallv
				rd[j] = displs[j] + i*blockSize;
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
* @param[in] 		counts     	Array containing the number of elements to be gathered from each process
* @param[in] 		displs     	Array of displacements
* @param[in] 		root     	Rank of the root process
*/
void TEST_DAL_gatherv( Data *data, long *counts, long *displs, int root )
{
	if( GET_ID() == root ) {
		return TEST_DAL_gathervReceive( data, counts, displs );
	}
	else {
		return TEST_DAL_gathervSend( data, root );
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

	long rcounts[n];
	long rdispls[n];
	int size = n*25;	//size of data to collect
	int r;

	//Initializing send counts randomly
	memset( rcounts, 0, n*sizeof(long) );
	for ( r=0; r<size; r++ )
		rcounts[rand()%n]++;

	//Computing displacements
	for ( r=0, i=0; i<n; i++ ) {
		rdispls[i] = r;
		r += rcounts[i];
	}

	//Data
	Data d;
	DAL_init( &d );

	if ( GET_ID() % 2 ) {
		SPD_ASSERT( DAL_allocArray( &d, rcounts[GET_ID()] ), "error allocating data..." );
	}
	else
		SPD_ASSERT( DAL_allocData( &d, rcounts[GET_ID()] ), "error allocating data..." );

	//tmp buffer to init data
	Data buffer;
	DAL_init( &buffer );
	SPD_ASSERT( DAL_allocArray( &buffer, rcounts[GET_ID()] ), "error allocating buffer..." );
	for( i=0; i<rcounts[GET_ID()]; i++ )
		buffer.array.data[i] = GET_ID();

	//Initializing data
	DAL_dataCopy( &buffer, &d );

	//Destroying tmp buffer
	DAL_destroy( &buffer );

	DAL_PRINT_DATA( &d, "This is what I had" );

	//Gather communication
	TEST_DAL_gatherv( &d, rcounts, rdispls, root );

	if ( GET_ID() == root ) {
		DAL_PRINT_DATA( &d, "This is what I got" );
	}

	DAL_destroy( &d );

	DAL_finalize( );
	return 0;
}

