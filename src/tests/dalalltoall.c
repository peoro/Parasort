
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
void TEST_DAL_splitBuffer( Data *buf, const int parts, Data *bufs )
{
	DAL_ASSERT( buf->medium == Array, buf, "Data should be of type Array" );
	DAL_ASSERT( buf->array.size % parts == 0, buf, "Data size should be a multiple of %d, but it's "DST, parts, buf->array.size );
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
* @param[in] 		count  		Number of elements to be sent/received to/from each process
*/
void TEST_DAL_alltoall( Data *data, dal_size_t count )
{
	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	DAL_ASSERT( globalBuf.array.size >= GET_N()*2, &globalBuf, "The global-buffer is too small for an alltoall communication (its size is "DST", but there are %d processes)", globalBuf.array.size, GET_N() );

	Data bufs[2];
	TEST_DAL_splitBuffer( &globalBuf, 2, bufs );
	Data *sendBuf = &bufs[0];
	Data *recvBuf = &bufs[1];

	//VALID ONLY FOR THIS TEST//
	sendBuf->array.size = GET_N();
	recvBuf->array.size = GET_N();
	////////////////////////////

	DAL_ASSERT( data->file.size >= sendBuf->array.size, data, "Data to be sent is too small, should be of type Array, but it's of type File" );

	int i, j;
	int blockSize = DAL_dataSize(sendBuf) / GET_N();

	for ( i=0; i<DAL_BLOCK_COUNT(data, sendBuf); i++ ) {
		int currCount = MIN( blockSize, (count-i*blockSize) );		//Number of elements to be sent to each process by MPI_Alltoall

		for ( j=0; j<GET_N(); j++ )
			DAL_dataCopyOS( data, j*count + i*blockSize, sendBuf, j*currCount, currCount );

		MPI_Alltoall( sendBuf->array.data, currCount, MPI_INT, recvBuf->array.data, currCount, MPI_INT, MPI_COMM_WORLD );

		for ( j=0; j<GET_N(); j++ )
			DAL_dataCopyOS( recvBuf, j*currCount, data, j*count + i*blockSize, currCount );
	}

	DAL_releaseGlobalBuffer( &globalBuf );
}


int main( int argc, char **argv )
{
	int i, n;

	DAL_initialize( &argc, &argv );

	n = GET_N();
	if( n < 2 ) {
		TESTS_ERROR( 1, "Use this with at least 2 processes!" );
	}

	int count = 1;		//number of elements to be sent to each process
	int size = n*count;	//size of data d

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
	TEST_DAL_alltoall( &d, count );

	DAL_PRINT_DATA( &d, "This is what I got" );

	DAL_destroy( &d );

	DAL_finalize( );
	return 0;
}

