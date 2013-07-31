
// just testing DAL_scatter primitive

#include "tests.h"
#include "../dal.h"
#include "../dal_internals.h"

#define MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define MAX(a,b) ( (a)>(b) ? (a) : (b) )

void TEST_DAL_scattervSend( Data *data, dal_size_t *counts, dal_size_t *displs )
{
	int sc[GET_N()];
	int sd[GET_N()];
	int i, j;

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	DAL_ASSERT( globalBuf.array.size >= GET_N(), &globalBuf, "The global-buffer is too small for a scatter communication (its size is "DST", but there are %d processes)", globalBuf.array.size, GET_N() );

	//VALID ONLY FOR THIS TEST//
	globalBuf.array.size = GET_N();
	////////////////////////////

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	dal_size_t max_count = 0;
	for ( i=0; i<GET_N(); i++ )
		if ( counts[i] > max_count )
			max_count = counts[i];
	MPI_Bcast( &max_count, 1, MPI_LONG_LONG, GET_ID(), MPI_COMM_WORLD );
	int num_iterations = max_count / blockSize + (max_count % blockSize > 0);
	int s, tmp;

	switch( data->medium ) {
		case File: {

			for ( i=0; i<num_iterations; i++ ) {

				for ( s=0, j=0; j<GET_N(); j++ ) {
					tmp = MIN( blockSize, (counts[j]-i*blockSize) );
					sc[j] =	tmp > 0 ? tmp : 0;	//Number of elements to be sent to process j by MPI_Alltoallv
					sd[j] = s;
					s += sc[j];

					if( sc[j] )
						DAL_dataCopyOS( data, displs[j] + i*blockSize, &globalBuf, sd[j], sc[j] );
				}
				MPI_Scatterv( globalBuf.array.data, sc, sd, MPI_INT, MPI_IN_PLACE, sc[GET_ID()], MPI_INT, GET_ID(), MPI_COMM_WORLD );
			}

			//TODO: resize root data (maybe a DAL_reallocData function would be useful)
			data->file.size = counts[GET_ID()];
			break;
		}
		case Array: {

			for ( i=0; i<num_iterations; i++ ) {

				for ( j=0; j<GET_N(); j++ ) {
					tmp = MIN( blockSize, (counts[j]-i*blockSize) );
					sc[j] =	tmp > 0 ? tmp : 0;	//Number of elements to be sent to process j by MPI_Alltoallv
					sd[j] = displs[j] + i*blockSize;
				}
				MPI_Scatterv( data->array.data, sc, sd, MPI_INT, MPI_IN_PLACE, sc[GET_ID()], MPI_INT, GET_ID(), MPI_COMM_WORLD );
			}
			SPD_ASSERT( DAL_reallocArray( data, counts[GET_ID()] ), "not enough memory to allocate data" );
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}

	DAL_releaseGlobalBuffer( &globalBuf );
}
void TEST_DAL_scattervReceive( Data *data, dal_size_t size, int root )
{
	int i, j;

	SPD_ASSERT( DAL_allocArray( data, size ), "not enough memory to allocate data" );
// 	SPD_ASSERT( DAL_allocData( data, size ), "not enough space to allocate data" );

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	//VALID ONLY FOR THIS TEST//
	globalBuf.array.size = GET_N();
	////////////////////////////

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	dal_size_t max_count;
	MPI_Bcast( &max_count, 1, MPI_LONG_LONG, root, MPI_COMM_WORLD );
	int num_iterations = max_count / blockSize + (max_count % blockSize > 0);
	int recvCount, tmp;

	switch( data->medium ) {
		case File: {

			for ( i=0; i<num_iterations; i++ ) {
				tmp = MIN( blockSize, size-i*blockSize );
				recvCount = tmp > 0 ? tmp : 0;
				MPI_Scatterv( NULL, NULL, NULL, MPI_INT, globalBuf.array.data, recvCount, MPI_INT, root, MPI_COMM_WORLD );

				if ( recvCount )
					DAL_dataCopyOS( &globalBuf, 0, data, i*blockSize, recvCount );
			}
			break;
		}
		case Array: {
			int recvDispl = 0;

			for ( i=0; i<num_iterations; i++ ) {
				tmp = MIN( blockSize, size-i*blockSize );
				recvCount = tmp > 0 ? tmp : 0;
				MPI_Scatterv( NULL, NULL, NULL, MPI_INT, data->array.data+recvDispl, recvCount, MPI_INT, root, MPI_COMM_WORLD );
				recvDispl += recvCount;
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
* @param[in,out] 	data  		Data to be scattered/received
* @param[in] 		counts     	Array containing the number of elements to be sent to each process
* @param[in] 		displs     	Array of displacements
* @param[in] 		root     	Rank of the root process
*/
void TEST_DAL_scatterv( Data *data, dal_size_t *counts, dal_size_t *displs, int root )
{
	if( GET_ID() == root ) {
		return TEST_DAL_scattervSend( data, counts, displs );
	}
	else {
		return TEST_DAL_scattervReceive( data, counts[GET_ID()], root );
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

	dal_size_t scounts[n];
	dal_size_t sdispls[n];
	int size = n*5;	//size of data d
	int s;

	//Initializing send counts randomly
	memset( scounts, 0, n*sizeof(dal_size_t) );
	for ( s=0; s<size; s++ )
		scounts[rand()%n]++;

	//Computing displacements
	for ( s=0, i=0; i<n; i++ ) {
		sdispls[i] = s;
		s += scounts[i];
	}

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
	TEST_DAL_scatterv( &d, scounts, sdispls, root );

	DAL_PRINT_DATA( &d, "This is what I got" );

	DAL_destroy( &d );

	DAL_finalize( );
	return 0;
}

