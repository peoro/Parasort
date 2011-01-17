
// just testing DAL_alltoall primitive

#include "tests.h"
#include "../dal.h"
#include "../dal_internals.h"

#define MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define MAX(a,b) ( (a)>(b) ? (a) : (b) )

/**
* @brief Split a Data into several parts
*
* @param[in] 	buf  		Data buffer to be split
* @param[in] 	parts  		Number of parts to split buf
* @param[out] 	bufs  		Array of Data as result of splitting
*/
void DAL_splitBuffer( Data *buf, const int parts, Data *bufs )
{
	DAL_ASSERT( buf->medium == Array, buf, "Data should be of type Array" );
	DAL_ASSERT( buf->array.size % parts == 0, buf, "Data size should be a multiple of %d, but it's %ld", parts, buf->array.size );
	int i;
	for ( i=0; i<parts; i++ ) {
		bufs[i] = *buf;
		bufs[i].array.data = buf->array.data+(buf->array.size/parts)*i;
		bufs[i].array.size /= parts;
	}
}

/**
* @brief Sends data from all to all processes
*
* @param[in,out] 	data  		Data to be sent and in which receive
* @param[in] 		scounts  	Array containing the number of elements to be sent to each process
* @param[in] 		sdispls     Array of displacements
* @param[in] 		rcounts  	Array containing the number of elements to be received from each process
* @param[in] 		rdispls     Array of displacements
*/
void TEST_DAL_alltoallv( Data *data, long *scounts, long *sdispls, long *rcounts, long *rdispls )
{
	int i, j;
	int sc[GET_N()];
	int sd[GET_N()];
	int rc[GET_N()];
	int rd[GET_N()];
	long rcount = 0;

	//Computing the total number of elements to be received
	for ( i=0; i<GET_N(); i++ )
		rcount += rcounts[i];

	//Data to store the received elements
	Data recvData;
	DAL_init( &recvData );
	SPD_ASSERT( DAL_allocData(&recvData, rcount), "not enough space to allocate data" );

	if ( data->medium == File || recvData.medium == File ) {
		Data globalBuf;
		DAL_acquireGlobalBuffer( &globalBuf );

		DAL_ASSERT( globalBuf.array.size >= GET_N()*2, &globalBuf, "The global-buffer is too small for an alltoall communication (its size is %ld, but there are %d processes)", globalBuf.array.size, GET_N() );

		Data bufs[2];
		DAL_splitBuffer( &globalBuf, 2, bufs );
		Data *sendBuf = &bufs[0];
		Data *recvBuf = &bufs[1];

		//VALID ONLY FOR THIS TEST//
		sendBuf->array.size = GET_N();
		recvBuf->array.size = GET_N();
		////////////////////////////

		int blockSize = DAL_dataSize(sendBuf) / GET_N();

		//Retrieving the number of iterations
		long max_count;
		MPI_Allreduce( &rcount, &max_count, 1, MPI_LONG, MPI_MAX, MPI_COMM_WORLD );
		int num_iterations = max_count / DAL_dataSize(sendBuf) + max_count % DAL_dataSize(sendBuf);
		int s, r, tmp;

		for ( i=0; i<num_iterations; i++ ) {

			for ( s=0, r=0, j=0; j<GET_N(); j++ ) {
				tmp = MIN( blockSize, (scounts[j]-i*blockSize) );
				sc[j] =	tmp > 0 ? tmp : 0;	//Number of elements to be sent to process j by MPI_Alltoallv
				sd[j] = s;
				s += sc[j];

				if( sc[j] )
					DAL_dataCopyOS( data, sdispls[j] + i*blockSize, sendBuf, sd[j], sc[j] );

				tmp = MIN( blockSize, (rcounts[j]-i*blockSize) );
				rc[j] =	tmp > 0 ? tmp : 0; 	//Number of elements to be received from process j by MPI_Alltoallv
				rd[j] = r;
				r += rc[j];
			}

			MPI_Alltoallv( sendBuf->array.data, sc, sd, MPI_INT, recvBuf->array.data, rc, rd, MPI_INT, MPI_COMM_WORLD );

			for ( j=0; j<GET_N(); j++ )
				if( rc[j] )
					DAL_dataCopyOS( recvBuf, rd[j], &recvData, rdispls[j] + i*blockSize, rc[j] );

		}

		DAL_releaseGlobalBuffer( &globalBuf );
	}
	else if (data->medium == Array && recvData.medium == Array ) {

		//TODO: Is it possible to avoid a double copy!?!?

		for ( i=0; i<GET_N(); i++ ) {
			sc[i] = scounts[i];
			sd[i] = sdispls[i];

			rc[i] = rcounts[i];
			rd[i] = rdispls[i];
		}

		MPI_Alltoallv( data->array.data, sc, sd, MPI_INT, recvData.array.data, rc, rd, MPI_INT, MPI_COMM_WORLD );
	}
	else
		DAL_UNSUPPORTED( data );

	DAL_destroy( data );
	*data = recvData;
}

int main( int argc, char **argv )
{
	int i, n;

	DAL_initialize( &argc, &argv );

	n = GET_N();
	if( n < 2 ) {
		TESTS_ERROR( 1, "Use this with at least 2 processes!" );
	}

	long scounts[n];
	long sdispls[n];
	long rcounts[n];
	long rdispls[n];
	int size = n;	//size of data d
	int s, r;

	//Initializing send counts randomly
	memset( scounts, 0, n*sizeof(long) );
	for ( s=0; s<size; s++ )
		scounts[rand()%n]++;

	// Informing all processes on the number of elements that will receive
	MPI_Alltoall( scounts, 1, MPI_LONG, rcounts, 1, MPI_LONG, MPI_COMM_WORLD );

	//Computing displacements
	for ( s=0, r=0, i=0; i<n; i++ ) {
		sdispls[i] = s;
		s += scounts[i];
		rdispls[i] = r;
		r += rcounts[i];
	}

	//Data to be sent
	Data d;
	DAL_init( &d );
// 	SPD_ASSERT( DAL_allocArray( &d, size ), "error allocating data..." );
	SPD_ASSERT( DAL_allocData( &d, size ), "error allocating data..." );

	//tmp buffer to init data
	Data buffer;
	DAL_init( &buffer );
	SPD_ASSERT( DAL_allocArray( &buffer, size ), "error allocating buffer..." );
	for( i=0; i<size; i++ )
		buffer.array.data[i] = GET_ID();

	//Initializing data
	DAL_dataCopy( &buffer, &d );

	//Destroying tmp buffer
	DAL_destroy( &buffer );

	//AlltoAll communication
	TEST_DAL_alltoallv( &d, scounts, sdispls, rcounts, rdispls );

	DAL_PRINT_DATA( &d, "This is what I got" );

	DAL_destroy( &d );

	DAL_finalize( );
	return 0;
}

