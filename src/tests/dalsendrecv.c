
// just testing send and receive

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
* @brief Sends and receives data from partner
*
* @param[in] sdata		Data to be sent
* @param[in] scount		Number of elements to be sent
* @param[in] sdispl		Displacement for the send buffer
* @param[in] rdata		Data buffer to store received elements
* @param[in] rcount		Max number of elements to be received
* @param[in] rdispl		Displacement for the receive buffer
* @param[in] partner	Rank of the partner process
*
* @returns Number of received integers
*/
dal_size_t TEST_DAL_sendrecv( Data *sdata, dal_size_t scount, dal_size_t sdispl, Data* rdata, dal_size_t rcount, dal_size_t rdispl, int partner )
{
	int i, sc, rc, tmp;
	dal_size_t recvCount = 0;
	MPI_Status 	status;

	if( rdata->medium == NoMedium ) {
		SPD_ASSERT( DAL_allocData( rdata, rdispl+rcount ), "not enough space to allocate data" );
	}
	else {
		SPD_ASSERT( DAL_reallocData( rdata, rdispl+rcount ), "not enough space to allocate data" );
	}
	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );
	Data bufs[2];
	TEST_DAL_splitBuffer( &globalBuf, 2, bufs );
	Data *sendBuf = &bufs[0];
	Data *recvBuf = &bufs[1];

	//VALID ONLY FOR THIS TEST//
	sendBuf->array.size = 1;
	recvBuf->array.size = 1;
	////////////////////////////

	//Retrieving the number of iterations
	int blockSize = DAL_dataSize(sendBuf);
	int num_iterations = rcount / blockSize + (rcount % blockSize > 0);

	for ( i=0; i<num_iterations; i++ ) {
		tmp = MIN( blockSize, (scount-i*blockSize) );		//Number of elements to be sent to the partner process by MPI_Sendrecv
		sc = tmp > 0 ? tmp : 0;
		tmp = MIN( blockSize, (rcount-i*blockSize) );		//Number of elements to be received from the partner process by MPI_Sendrecv
		rc = tmp > 0 ? tmp : 0;

		if ( sc )
			DAL_dataCopyOS( sdata, sdispl + i*blockSize, sendBuf, 0, sc );

		MPI_Sendrecv( sendBuf->array.data, sc, MPI_INT, partner, 100, recvBuf->array.data, rc, MPI_INT, partner, 100, MPI_COMM_WORLD, &status );
		MPI_Get_count( &status, MPI_INT, &rc );

		if ( rc ) {
			DAL_dataCopyOS( recvBuf, 0, rdata, rdispl + recvCount, rc );
			recvCount += rc;
		}
	}
	SPD_ASSERT( DAL_reallocData( rdata, rdispl+recvCount ), "not enough space to allocate data" );

	DAL_releaseGlobalBuffer( &globalBuf );

	return recvCount;
}



int main( int argc, char **argv )
{
// 	int i, n;
// 	Data d;
// 	DAL_init( &d );
//
// 	DAL_initialize( &argc, &argv );
//
// 	n = GET_N();
// 	if( n < 2 ) {
// 		TESTS_ERROR( 1, "Use this with at least 2 processes!" );
// 	}
//
// 	if( GET_ID() == 0 ) {
// 		Data tmp;
// 		DAL_init( &tmp );
// 		SPD_ASSERT( DAL_allocArray( &tmp, 1 ), "error allocating array..." );
// 		tmp.array.data[0] = 666;
//
// 		SPD_ASSERT( DAL_allocData( &d, 1 ), "error allocating data..." );
//
// 		DAL_dataCopy( &tmp, &d );
//
// 		DAL_destroy( &tmp );
//
// 		DAL_send( &d, GET_ID()+1 );
// 	}
// 	else if( GET_ID() < GET_N()-1 ) {
// 		DAL_receive( &d, 1, GET_ID()-1 );
// 		DAL_send( &d, GET_ID()+1 );
// 	}
// 	else /* GET_ID() == GET_N()-1 */ {
// 		DAL_receive( &d, 1, GET_ID()-1 );
// 	}
//
// 	DAL_PRINT_DATA( &d, "This is what I got" );
//
// 	DAL_destroy( &d );
//
// 	DAL_finalize( );

	int i, n;

	DAL_initialize( &argc, &argv );

	n = GET_N();
	if( n < 2 ) {
		TESTS_ERROR( 1, "Use this with at least 2 processes!" );
	}

	Data sendData;
	DAL_init( &sendData );

	Data recvData;
	DAL_init( &recvData );

	SPD_ASSERT( DAL_allocData( &sendData, 1 ), "error allocating data..." );

	Data tmp;
	DAL_init( &tmp );
	SPD_ASSERT( DAL_allocArray( &tmp, 1 ), "error allocating array..." );
	tmp.array.data[0] = GET_ID();

	DAL_dataCopy( &tmp, &sendData );

	DAL_PRINT_DATA( &sendData, "This is what I had" );

	TEST_DAL_sendrecv( &sendData, 1, 0, &recvData, 1, 0, (GET_ID() + 1)%n );

	DAL_PRINT_DATA( &recvData, "This is what I got" );

	DAL_destroy( &sendData );
	DAL_destroy( &recvData );

	DAL_finalize( );

	return 0;
}


