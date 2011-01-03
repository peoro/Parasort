
#include<assert.h>
#include "communication.h"


// TODO: pure debugging, move in debug.c ...
#include <execinfo.h>
void print_trace( void )
{
	void *array[64];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace( array, 64 );
	strings = backtrace_symbols( array, size );

	printf( "\n" );
	printf( "Tracebak (%zd stack frames):\n", size );

	for( i = 0; i < size; ++ i ) {
		printf( "  %s\n", strings[i] );
	}
	printf( "\n" );

	free( strings );
}


long GET_FILE_SIZE( const char *path )
{
	FILE *f;
	long len;

	f = fopen( path, "rb" );
	if( ! f ) {
		return -1;
	}

	fseek( f, 0, SEEK_END );
	len = ftell( f );
	// rewind( f );

	fclose( f );

	return len;
}
const char *DAL_mediumName( enum Medium m )
{
	switch( m ) {
		case NoMedium:
			return "no medium";
		case File:
			return "file [disk]";
		case Array:
			return "array [primary memory]";
		default:
			return "unknown medium code";
	}
}
char * DAL_dataToString( Data *d, char *s, int size )
{
	switch( d->medium ) {
		case NoMedium:
			strncpy( s, DAL_mediumName(d->medium), size );
			break;
		case File:
			snprintf( s, size, "\"%s\" [file on disk] of %ld bytes", d->file.name, GET_FILE_SIZE(d->file.name) );
			break;
		case Array:
			snprintf( s, size, "array [on principal memory @ %p] of %ld bytes", d->array.data, d->array.size );
			break;
		default:
			snprintf( s, size, "Unknow medium code %d [%s]", d->medium, DAL_mediumName(d->medium) );
	}
	return s;
}




bool DAL_isInitialized( Data *data )
{
	return data->medium == NoMedium;
}
void DAL_init( Data *data )
{
	data->medium = NoMedium;
}

void DAL_destroy( Data *data )
{
	switch( data->medium ) {
		case NoMedium: {
			break;
		}
		case File: {
			UNSUPPORTED_DATA( data );
			break;
		}
		case Array: {
			free( data->array.data );
			break;
		}
		default:
			UNSUPPORTED_DATA( data );
	}
	DAL_init( data );
}

bool DAL_allocArray( Data *data, int size )
{
	ASSERT_DATA( DAL_isInitialized(data), data, "data should have been initialized" );

	data->array.data = (int*) malloc( size * sizeof(int) );
	if( ! data->array.data ) {
		return 0;
	}
	data->array.size = size;

	data->medium = Array;

	return 1;
}

bool DAL_reallocArray ( Data *data, int size )
{
	ASSERT_DATA( data->medium == Array, data, "only Array data can be reallocated" );

	data->array.data = (int*) realloc( data->array.data, size * sizeof(int) );
	if( ! data->array.data ) {
		return 0;
	}
	data->array.size = size;

	data->medium = Array;

	return 1;
}



/***************************************************************************************************************/
/************************************* [Data] Communication Primitives *****************************************/
/***************************************************************************************************************/
/**
* @brief Sends data to dest
*
* @param[in] ti       	The test info
* @param[in] data  		Data to be sent
* @param[in] dest     	Rank of the receiver process
*/
void DAL_send( const TestInfo *ti, Data *data, int dest )
{
	switch( data->medium ) {
		case File: {
			UNSUPPORTED_DATA( data );
			break;
		}
		case Array: {
			MPI_Send( data->array.data, data->array.size, MPI_INT, dest, 0, MPI_COMM_WORLD );
			break;
		}
		default:
			UNSUPPORTED_DATA( data );
	}
}

/**
* @brief Receives data from source
*
* @param[in] ti       	The test info
* @param[in] data		Data buffer to store received elements
* @param[in] size  		Max number of integers to be received
* @param[in] source    	Rank of the sender process
*/
void DAL_receive( const TestInfo *ti, Data *data, long size, int source )
{
	MPI_Status 	stat;
	assert( DAL_allocArray( data, size ) );
	MPI_Recv( data->array.data, size, MPI_INT, source, 0, MPI_COMM_WORLD, &stat );
}


/**
* @brief Sends and receives data from partner
*
* @param[in] ti       	The test info
* @param[in] sdata		Data to be sent
* @param[in] scount		Number of integers to be sent
* @param[in] sdispl		Displacement for the send buffer
* @param[in] rdata		Data buffer to store received elements
* @param[in] rcount		Max number of integers to be received
* @param[in] rdispl		Displacement for the receive buffer
* @param[in] partner	Rank of the partner process
*
* @returns Number of received integers
*/
long DAL_sendrecv( const TestInfo *ti, Data *sdata, long scount, long sdispl, Data* rdata, long rcount, long rdispl, int partner )
{
	int recvCount = rcount;
	int sendCount = scount;
	int recvDispl = rdispl;
	int sendDispl = sdispl;

	if ( rdata->medium == NoMedium )
		assert( DAL_allocArray( rdata, recvDispl+recvCount ) );
	else
		assert( DAL_reallocArray( rdata, recvDispl+recvCount ) );

	MPI_Status 	status;
	MPI_Sendrecv( sdata->array.data+sendDispl, sendCount, MPI_INT, partner, 100, rdata->array.data+recvDispl, recvCount, MPI_INT, partner, 100, MPI_COMM_WORLD, &status );

	MPI_Get_count( &status, MPI_INT, &recvCount );
	if ( recvCount || recvDispl )
		assert( DAL_reallocArray( rdata, recvDispl+recvCount ) );
	else
		DAL_destroy( rdata );

	rcount = recvCount;
	return rcount;
}






void DAL_scatterSend( const TestInfo *ti, Data *data )
{
	switch( data->medium ) {
		case File: {
			UNSUPPORTED_DATA( data );
			break;
		}
		case Array: {
			assert( data->array.size % GET_N( ti ) == 0 );
			MPI_Scatter( data->array.data, data->array.size/GET_N( ti ), MPI_INT, MPI_IN_PLACE, data->array.size/GET_N( ti ), MPI_INT, GET_ID( ti ), MPI_COMM_WORLD );
			assert( DAL_reallocArray( data, data->array.size/GET_N( ti ) ) );
			break;
		}
		default:
			UNSUPPORTED_DATA( data );
	}
}
void DAL_scatterReceive( const TestInfo *ti, Data *data, long size, int root )
{
	assert( DAL_allocArray( data, size ) );
	MPI_Scatter( NULL, 0, MPI_INT, data->array.data, size, MPI_INT, root, MPI_COMM_WORLD );
}
/**
* @brief Scatters data among all processes
*
* @param[in] ti       	The test info
* @param[in] data  		Data to be scattered
* @param[in] size     	Number of integers per process
* @param[in] root     	Rank of the root process
*/
void DAL_scatter( const TestInfo *ti, Data *data, long size, int root )
{
	if( GET_ID( ti ) == root ) {
		return DAL_scatterSend( ti, data );
	}
	else {
		return DAL_scatterReceive( ti, data, size, root );
	}
}





void DAL_gatherSend( const TestInfo *ti, Data *data, int root )
{
	switch( data->medium ) {
		case File: {
			UNSUPPORTED_DATA( data );
			break;
		}
		case Array: {
			MPI_Gather( data->array.data, data->array.size, MPI_INT, NULL, 0, MPI_INT, root, MPI_COMM_WORLD );
			break;
		}
		default:
			UNSUPPORTED_DATA( data );
	}
}
void DAL_gatherReceive( const TestInfo *ti, Data *data, long size )
{
	assert( size % GET_N( ti ) == 0 );
	assert( DAL_reallocArray( data, size ) );
	MPI_Gather( MPI_IN_PLACE, size/GET_N( ti ), MPI_INT, data->array.data, size/GET_N( ti ), MPI_INT, GET_ID( ti ), MPI_COMM_WORLD );
}
/**
* @brief Gathers data from all processes
*
* @param[in] 		ti       	The test info
* @param[in,out] 	data  		Data to be gathered/sent
* @param[in] 		size     	Number of integers to be gathered
* @param[in] 		root     	Rank of the root process
*/
void DAL_gather( const TestInfo *ti, Data *data, long size, int root )
{
	if( GET_ID( ti ) == root ) {
		return DAL_gatherReceive( ti, data, size );
	}
	else {
		return DAL_gatherSend( ti, data, root );
	}
}







void DAL_scattervSend( const TestInfo *ti, Data *data, long *sizes, long *displs )
{
	switch( data->medium ) {
		case File: {
			UNSUPPORTED_DATA( data );
			break;
		}
		case Array: {
			int scounts[GET_N( ti )];
			int sdispls[GET_N( ti )];
			int i;

			for ( i=0; i<GET_N( ti ); i++ ) {
				scounts[i] = sizes[i];
				sdispls[i] = displs[i];
			}
		 	MPI_Scatterv( data->array.data, scounts, sdispls, MPI_INT, MPI_IN_PLACE, sizes[0], MPI_INT, GET_ID( ti ), MPI_COMM_WORLD );
			assert( DAL_reallocArray( data, data->array.size/GET_N( ti ) ) );
			break;
		}
		default:
			UNSUPPORTED_DATA( data );
	}
}
void DAL_scattervReceive( const TestInfo *ti, Data *data, long size, int root )
{
	assert( DAL_allocArray( data, size ) );
 	MPI_Scatterv( NULL, NULL, NULL, MPI_INT, data->array.data, size, MPI_INT, root, MPI_COMM_WORLD );
}
/**
* @brief Scatters data among all processes
*
* @param[in] 		ti       	The test info
* @param[in,out] 	data  		Data to be scattered/received
* @param[in] 		sizes     	Array containing the number of integers to be sent to each process
* @param[in] 		displs     	Array of displacements
* @param[in] 		root     	Rank of the root process
*/
void DAL_scatterv( const TestInfo *ti, Data *data, long *sizes, long *displs, int root )
{
	if( GET_ID( ti ) == root ) {
		return DAL_scattervSend( ti, data, sizes, displs );
	}
	else {
		return DAL_scattervReceive( ti, data, GET_LOCAL_M(ti), root );
	}
}




void DAL_gathervSend( const TestInfo *ti, Data *data, int root )
{
	switch( data->medium ) {
		case File: {
			UNSUPPORTED_DATA( data );
			break;
		}
		case Array: {
			MPI_Gatherv( data->array.data, data->array.size, MPI_INT, NULL, NULL, NULL, MPI_INT, root, MPI_COMM_WORLD );
			break;
		}
		default:
			UNSUPPORTED_DATA( data );
	}
}
void DAL_gathervReceive( const TestInfo *ti, Data *data, long *sizes, long *displs )
{
	int rcounts[GET_N( ti )];
	int rdispls[GET_N( ti )];
	int i;

	for ( i=0; i<GET_N( ti ); i++ ) {
		rcounts[i] = sizes[i];
		rdispls[i] = displs[i];
	}
	assert( DAL_reallocArray( data, GET_M( ti ) ) );
	MPI_Gatherv( MPI_IN_PLACE, rcounts[GET_ID( ti )], MPI_INT, data->array.data, rcounts, rdispls, MPI_INT, GET_ID( ti ), MPI_COMM_WORLD );
}
/**
* @brief Gathers data from all processes
*
* @param[in] 		ti       	The test info
* @param[in,out] 	data  		Data to be gathered/sent
* @param[in] 		sizes     	Array containing the number of integers to be gathered from each process
* @param[in] 		displs     	Array of displacements
* @param[in] 		root     	Rank of the root process
*/
void DAL_gatherv( const TestInfo *ti, Data *data, long *sizes, long *displs, int root )
{
	if( GET_ID( ti ) == root ) {
		return DAL_gathervReceive( ti, data, sizes, displs );
	}
	else {
		return DAL_gathervSend( ti, data, root );
	}
}







/**
* @brief Sends data from all to all processes
*
* @param[in] 		ti       	The test info
* @param[in,out] 	data  		Data to be sent/received
* @param[in] 		size  		Number of integers to be sent/received to/from each process
*/
void DAL_alltoall( const TestInfo *ti, Data *data, long size )
{
// 	UNSUPPORTED_DATA( data );
	int count = size;
	int i;

	Data *recvData = (Data*)malloc( sizeof(Data) );
	DAL_init( recvData );
	assert( recvData != NULL && DAL_allocArray(recvData, count) );

	MPI_Alltoall( data->array.data, count, MPI_INT, recvData, count, MPI_INT, MPI_COMM_WORLD );

	DAL_destroy( data );
	data->medium = Array;
	data->array = recvData->array;
}










/**
* @brief Sends data from all to all processes
*
* @param[in] 		ti       	The test info
* @param[in,out] 	data  		Data to be sent/received
* @param[in] 		sendSizes  	Array containing the number of integers to be sent to each process
* @param[in] 		sdispls     Array of displacements
* @param[in] 		recvSizes  	Array containing the number of integers to be received from each process
* @param[in] 		rdispls     Array of displacements
*/
void DAL_alltoallv( const TestInfo *ti, Data *data, long *sendSizes, long *sdispls, long *recvSizes, long *rdispls )
{
// 	UNSUPPORTED_DATA( data );

	int scounts[GET_N( ti )];
	int sd[GET_N( ti )];
	int rcounts[GET_N( ti )];
	int rd[GET_N( ti )];
	int rcount = 0;
	int i;

	for ( i=0; i<GET_N( ti ); i++ ) {
		scounts[i] = sendSizes[i];
		sd[i] = sdispls[i];

		rcounts[i] = recvSizes[i];
		rd[i] = rdispls[i];

		rcount += rcounts[i];
	}
	Data *recvData = (Data*)malloc( sizeof(Data) );
	DAL_init( recvData );
	assert( recvData != NULL && DAL_allocArray(recvData, rcount) );

	MPI_Alltoallv( data->array.data, scounts, sd, MPI_INT, recvData->array.data, rcounts, rd, MPI_INT, MPI_COMM_WORLD );

	DAL_destroy( data );
	data->medium = Array;
	data->array = recvData->array;
}





void DAL_bcastSend( const TestInfo *ti, Data *data )
{
	switch( data->medium ) {
		case File: {
			UNSUPPORTED_DATA( data );
			break;
		}
		case Array: {
			MPI_Bcast( data->array.data, data->array.size, MPI_INT, GET_ID( ti ), MPI_COMM_WORLD );
			break;
		}
		default:
			UNSUPPORTED_DATA( data );
	}
}
void DAL_bcastReceive( const TestInfo *ti, Data *data, long size, int root )
{
	assert( DAL_allocArray( data, size ) );
	MPI_Bcast( data->array.data, size, MPI_INT, root, MPI_COMM_WORLD );
}
/**
* @brief Broadcasts data to processes
*
* @param[in] ti       	The test info
* @param[in] data  		Data to be scattered
* @param[in] size     	Number of integers per process
* @param[in] root     	Rank of the root process
*/
void DAL_bcast( const TestInfo *ti, Data *data, long size, int root )
{
	if( GET_ID( ti ) == root ) {
		return DAL_bcastSend( ti, data );
	}
	else {
		return DAL_bcastReceive( ti, data, size, root );
	}
}
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/********************************* [Integer] Communication Primitives ******************************************/
/***************************************************************************************************************/

/**
* @brief Sends array to dest
*
* @param[in] ti         The test info
* @param[in] array      Pointer to an array of integers to be sent
* @param[in] size       Length of the array array
* @param[in] dest       Rank of the receiver process
*/
void DAL_i_send( const TestInfo *ti, int **array, int size, int dest )
{
    MPI_Send( *array, size, MPI_INT, dest, 0, MPI_COMM_WORLD );
}

/**
* @brief Receives array from source
*
* @param[in] ti         The test info
* @param[in] array      Array buffer to store received elements
* @param[in] size       Max number of integers to be received
* @param[in] source     Rank of the sender process
*
* @returns Number of received integers
*/
int DAL_i_receive( const TestInfo *ti, int **array, int size, int source )
{
    MPI_Status  status;
    *array = (int*)malloc( size * sizeof(int) );
    assert( *array != NULL );
    MPI_Recv( *array, size, MPI_INT, source, 0, MPI_COMM_WORLD, &status );

    MPI_Get_count( &status, MPI_INT, &size );

    return size;
}





/**
* @brief Sends and receives array from partner
*
* @param[in] ti         The test info
* @param[in] sarray     Pointer to an array of integers to be sent
* @param[in] scount     Number of integers to be sent
* @param[in] sdispl     Displacement for the send buffer
* @param[in] rarray     Buffer to store received elements
* @param[in] rcount     Max number of integers to be received
* @param[in] rdispl     Displacement for the receive buffer
* @param[in] partner    Rank of the partner process
*
* @returns Number of received integers
*/
int DAL_i_sendrecv( const TestInfo *ti, int **sarray, int scount, int sdispl, int **rarray, int rcount, int rdispl, int partner )
{
    *rarray = (int*)realloc( *rarray, (rdispl+rcount)*sizeof(int) );
    assert( *rarray != NULL );

    MPI_Status  status;
    MPI_Sendrecv( *sarray+sdispl, scount, MPI_INT, partner, 100, *rarray+rdispl, rcount, MPI_INT, partner, 100, MPI_COMM_WORLD, &status );

    MPI_Get_count( &status, MPI_INT, &rcount );
    if ( rcount || rdispl ) {
        *rarray = (int*)realloc( *rarray, (rdispl+rcount)*sizeof(int) );
        assert( *rarray != NULL );
    }
    else
        free( *rarray );

    return rcount;
}







void DAL_i_scatterSend( const TestInfo *ti, int **array, int size )
{
    assert( size % GET_N( ti ) == 0 );
    MPI_Scatter( *array, size, MPI_INT, MPI_IN_PLACE, size, MPI_INT, GET_ID( ti ), MPI_COMM_WORLD );

    *array = (int*)realloc( *array, size*sizeof(int) );
    assert( *array != NULL );
}
void DAL_i_scatterReceive( const TestInfo *ti, int **array, int size, int root )
{
    *array = (int*)malloc( size * sizeof(int) );
    assert( *array != NULL );
    MPI_Scatter( NULL, 0, MPI_INT, *array, size, MPI_INT, root, MPI_COMM_WORLD );
}
/**
* @brief Scatters array among all processes
*
* @param[in] ti         The test info
* @param[in] array      Pointer to an array of integers to be scattered
* @param[in] size       Number of integers per process
* @param[in] root       Rank of the root process
*/
void DAL_i_scatter( const TestInfo *ti, int **array, int size, int root )
{
    if( GET_ID( ti ) == root ) {
        return DAL_i_scatterSend( ti, array, size );
    }
    else {
        return DAL_i_scatterReceive( ti, array, size, root );
    }
}







void DAL_i_gatherSend( const TestInfo *ti, int **array, int size, int root )
{
    MPI_Gather( *array, size, MPI_INT, NULL, 0, MPI_INT, root, MPI_COMM_WORLD );
}
void DAL_i_gatherReceive( const TestInfo *ti, int **array, int size )
{
    assert( size % GET_N( ti ) == 0 );
    *array = (int*)realloc( *array, size*sizeof(int) );
    assert( *array != NULL );
    MPI_Gather( MPI_IN_PLACE, size/GET_N( ti ), MPI_INT, *array, size/GET_N( ti ), MPI_INT, GET_ID( ti ), MPI_COMM_WORLD );
}
/**
* @brief Gathers array from all processes
*
* @param[in]        ti          The test info
* @param[in,out]    array       Pointer to an array of integers to be gathered/sent
* @param[in]        size        Number of integers to be gathered
* @param[in]        root        Rank of the root process
*/
void DAL_i_gather( const TestInfo *ti, int **array, int size, int root )
{
    if( GET_ID( ti ) == root ) {
        return DAL_i_gatherReceive( ti, array, size );
    }
    else {
        return DAL_i_gatherSend( ti, array, size/GET_N( ti ), root );
    }
}








void DAL_i_scattervSend( const TestInfo *ti, int **array, int *sizes, int *displs )
{
    MPI_Scatterv( *array, sizes, displs, MPI_INT, MPI_IN_PLACE, sizes[0], MPI_INT, GET_ID( ti ), MPI_COMM_WORLD );

    *array = (int*)realloc( *array, sizes[0]*sizeof(int) );
    assert( *array != NULL );
}
void DAL_i_scattervReceive( const TestInfo *ti, int **array, int size, int root )
{
    *array = (int*)malloc( size * sizeof(int) );
    assert( *array != NULL );
    MPI_Scatterv( NULL, NULL, NULL, MPI_INT, *array, size, MPI_INT, root, MPI_COMM_WORLD );
}
/**
* @brief Scatters array among all processes
*
* @param[in]        ti          The test info
* @param[in,out]    array       Pointer to an array of integers to be scattered/received
* @param[in]        sizes       Array containing the number of integers to be sent to each process
* @param[in]        displs      Array of displacements
* @param[in]        root        Rank of the root process
*/
void DAL_i_scatterv( const TestInfo *ti, int **array, int *sizes, int *displs, int root )
{
    if( GET_ID( ti ) == root ) {
        return DAL_i_scattervSend( ti, array, sizes, displs );
    }
    else {
        return DAL_i_scattervReceive( ti, array, GET_LOCAL_M(ti), root );
    }
}








void DAL_i_gathervSend( const TestInfo *ti, int **array, int size, int root )
{
    MPI_Gatherv( *array, size, MPI_INT, NULL, NULL, NULL, MPI_INT, root, MPI_COMM_WORLD );
}
void DAL_i_gathervReceive( const TestInfo *ti, int **array, int *sizes, int *displs )
{
    *array = (int*)realloc( *array, GET_M( ti )*sizeof(int) );
    assert( *array != NULL );
    MPI_Gatherv( MPI_IN_PLACE, sizes[GET_ID( ti )], MPI_INT, *array, sizes, displs, MPI_INT, GET_ID( ti ), MPI_COMM_WORLD );
}
/**
* @brief Gathers array from all processes
*
* @param[in]        ti          The test info
* @param[in,out]    array       Pointer to an array of integers to be gathered/sent
* @param[in]        sizes       Array containing the number of integers to be gathered from each process
* @param[in]        displs      Array of displacements
* @param[in]        root        Rank of the root process
*/
void DAL_i_gatherv( const TestInfo *ti, int **array, int *sizes, int *displs, int root )
{
    if( GET_ID( ti ) == root ) {
        return DAL_i_gathervReceive( ti, array, sizes, displs );
    }
    else {
        return DAL_i_gathervSend( ti, array, sizes[0], root );
    }
}






/**
* @brief Sends data from all to all processes
*
* @param[in] 		ti       	The test info
* @param[in,out] 	data  		Data to be sent/received
* @param[in] 		size  		Number of integers to be sent/received to/from each process
*/
void DAL_i_alltoall( const TestInfo *ti, int **array, int size )
{
    int *recvArray = (int*)malloc( size * GET_N( ti ) * sizeof(int) );
    assert( recvArray != NULL );

	MPI_Alltoall( *array, size, MPI_INT, recvArray, size, MPI_INT, MPI_COMM_WORLD );

    free( *array );
    *array = recvArray;
}







/**
* @brief Sends array from all to all processes
*
* @param[in]        ti          The test info
* @param[in,out]    array       Pointer to an array of integers to be sent/received
* @param[in]        sendSizes   Array containing the number of integers to be sent to each process
* @param[in]        sdispls     Array of displacements
* @param[in]        recvSizes   Array containing the number of integers to be received from each process
* @param[in]        rdispls     Array of displacements
*/
void DAL_i_alltoallv( const TestInfo *ti, int **array, int *sendSizes, int *sdispls, int *recvSizes, int *rdispls )
{
    int rcount = 0;
    int i;

    for ( i=0; i<GET_N( ti ); i++ ) {
        rcount += recvSizes[i];
    }
    int *recvArray = (int*)malloc( rcount*sizeof(int) );
    assert( recvArray != NULL );

    MPI_Alltoallv( *array, sendSizes, sdispls, MPI_INT, recvArray, recvSizes, rdispls, MPI_INT, MPI_COMM_WORLD );

    free( *array );
    *array = recvArray;
}






void DAL_i_bcastSend( const TestInfo *ti, int **array, int size )
{
	MPI_Bcast( *array, size, MPI_INT, GET_ID( ti ), MPI_COMM_WORLD );
}
void DAL_i_bcastReceive( const TestInfo *ti, int **array, int size, int root )
{
    *array = (int*)malloc( size * sizeof(int) );
    assert( *array != NULL );
    MPI_Bcast( *array, size, MPI_INT, root, MPI_COMM_WORLD );
}
/**
* @brief Broadcasts array to processes
*
* @param[in] ti         The test info
* @param[in] array      Pointer to an array of integers to be broadcast
* @param[in] size       Number of integers per process
* @param[in] root       Rank of the root process
*/
void DAL_i_bcast( const TestInfo *ti, int **array, int size, int root )
{
    if( GET_ID( ti ) == root ) {
        return DAL_i_bcastSend( ti, array, size );
    }
    else {
        return DAL_i_bcastReceive( ti, array, size, root );
    }
}

/*--------------------------------------------------------------------------------------------------------------*/









/***************************************************************************************************************/
/*********************************** [Long] Communication Primitives *******************************************/
/***************************************************************************************************************/

/**
* @brief Sends array to dest
*
* @param[in] ti         The test info
* @param[in] array      Pointer to an array of longs to be sent
* @param[in] size       Length of the array array
* @param[in] dest       Rank of the receiver process
*/
void DAL_l_send( const TestInfo *ti, long **array, int size, int dest )
{
    MPI_Send( *array, size, MPI_LONG, dest, 0, MPI_COMM_WORLD );
}

/**
* @brief Receives array from source
*
* @param[in] ti         The test info
* @param[in] array      Array buffer to store received elements
* @param[in] size       Max number of longs to be received
* @param[in] source     Rank of the sender process
*
* @returns Number of received longs
*/
int DAL_l_receive( const TestInfo *ti, long **array, int size, int source )
{
    MPI_Status  status;
    *array = (long*)malloc( size * sizeof(long) );
    assert( *array != NULL );
    MPI_Recv( *array, size, MPI_LONG, source, 0, MPI_COMM_WORLD, &status );

    MPI_Get_count( &status, MPI_LONG, &size );

    return size;
}





/**
* @brief Sends and receives array from partner
*
* @param[in] ti         The test info
* @param[in] sarray     Pointer to an array of longs to be sent
* @param[in] scount     Number of longs to be sent
* @param[in] sdispl     Displacement for the send buffer
* @param[in] rarray     Buffer to store received elements
* @param[in] rcount     Max number of longs to be received
* @param[in] rdispl     Displacement for the receive buffer
* @param[in] partner    Rank of the partner process
*
* @returns Number of received longs
*/
int DAL_l_sendrecv( const TestInfo *ti, long **sarray, int scount, int sdispl, long **rarray, int rcount, int rdispl, int partner )
{
    *rarray = (long*)realloc( *rarray, (rdispl+rcount)*sizeof(long) );
    assert( *rarray != NULL );

    MPI_Status  status;
    MPI_Sendrecv( *sarray+sdispl, scount, MPI_LONG, partner, 100, *rarray+rdispl, rcount, MPI_LONG, partner, 100, MPI_COMM_WORLD, &status );

    MPI_Get_count( &status, MPI_LONG, &rcount );
    if ( rcount || rdispl ) {
        *rarray = (long*)realloc( *rarray, (rdispl+rcount)*sizeof(long) );
        assert( *rarray != NULL );
    }
    else
        free( *rarray );

    return rcount;
}







void DAL_l_scatterSend( const TestInfo *ti, long **array, int size )
{
    assert( size % GET_N( ti ) == 0 );
    MPI_Scatter( *array, size, MPI_LONG, MPI_IN_PLACE, size, MPI_LONG, GET_ID( ti ), MPI_COMM_WORLD );

    *array = (long*)realloc( *array, size*sizeof(long) );
    assert( *array != NULL );
}
void DAL_l_scatterReceive( const TestInfo *ti, long **array, int size, int root )
{
    *array = (long*)malloc( size * sizeof(long) );
    assert( *array != NULL );
    MPI_Scatter( NULL, 0, MPI_LONG, *array, size, MPI_LONG, root, MPI_COMM_WORLD );
}
/**
* @brief Scatters array among all processes
*
* @param[in] ti         The test info
* @param[in] array      Pointer to an array of longs to be scattered
* @param[in] size       Number of longs per process
* @param[in] root       Rank of the root process
*/
void DAL_l_scatter( const TestInfo *ti, long **array, int size, int root )
{
    if( GET_ID( ti ) == root ) {
        return DAL_l_scatterSend( ti, array, size );
    }
    else {
        return DAL_l_scatterReceive( ti, array, size, root );
    }
}







void DAL_l_gatherSend( const TestInfo *ti, long **array, int size, int root )
{
    MPI_Gather( *array, size, MPI_LONG, NULL, 0, MPI_LONG, root, MPI_COMM_WORLD );
}
void DAL_l_gatherReceive( const TestInfo *ti, long **array, int size )
{
    assert( size % GET_N( ti ) == 0 );
    *array = (long*)realloc( *array, size*sizeof(long) );
    assert( *array != NULL );
    MPI_Gather( MPI_IN_PLACE, size/GET_N( ti ), MPI_LONG, *array, size/GET_N( ti ), MPI_LONG, GET_ID( ti ), MPI_COMM_WORLD );
}
/**
* @brief Gathers array from all processes
*
* @param[in]        ti          The test info
* @param[in,out]    array       Pointer to an array of longs to be gathered/sent
* @param[in]        size        Number of longs to be gathered
* @param[in]        root        Rank of the root process
*/
void DAL_l_gather( const TestInfo *ti, long **array, int size, int root )
{
    if( GET_ID( ti ) == root ) {
        return DAL_l_gatherReceive( ti, array, size );
    }
    else {
        return DAL_l_gatherSend( ti, array, size/GET_N( ti ), root );
    }
}








void DAL_l_scattervSend( const TestInfo *ti, long **array, int *sizes, int *displs )
{
    MPI_Scatterv( *array, sizes, displs, MPI_LONG, MPI_IN_PLACE, sizes[0], MPI_LONG, GET_ID( ti ), MPI_COMM_WORLD );

    *array = (long*)realloc( *array, sizes[0]*sizeof(long) );
    assert( *array != NULL );
}
void DAL_l_scattervReceive( const TestInfo *ti, long **array, int size, int root )
{
    *array = (long*)malloc( size * sizeof(long) );
    assert( *array != NULL );
    MPI_Scatterv( NULL, NULL, NULL, MPI_LONG, *array, size, MPI_LONG, root, MPI_COMM_WORLD );
}
/**
* @brief Scatters array among all processes
*
* @param[in]        ti          The test info
* @param[in,out]    array       Pointer to an array of longs to be scattered/received
* @param[in]        sizes       Array containing the number of longs to be sent to each process
* @param[in]        displs      Array of displacements
* @param[in]        root        Rank of the root process
*/
void DAL_l_scatterv( const TestInfo *ti, long **array, int *sizes, int *displs, int root )
{
    if( GET_ID( ti ) == root ) {
        return DAL_l_scattervSend( ti, array, sizes, displs );
    }
    else {
        return DAL_l_scattervReceive( ti, array, GET_LOCAL_M(ti), root );
    }
}








void DAL_l_gathervSend( const TestInfo *ti, long **array, int size, int root )
{
    MPI_Gatherv( *array, size, MPI_LONG, NULL, NULL, NULL, MPI_LONG, root, MPI_COMM_WORLD );
}
void DAL_l_gathervReceive( const TestInfo *ti, long **array, int *sizes, int *displs )
{
    *array = (long*)realloc( *array, GET_M( ti )*sizeof(long) );
    assert( *array != NULL );
    MPI_Gatherv( MPI_IN_PLACE, sizes[GET_ID( ti )], MPI_LONG, *array, sizes, displs, MPI_LONG, GET_ID( ti ), MPI_COMM_WORLD );
}
/**
* @brief Gathers array from all processes
*
* @param[in]        ti          The test info
* @param[in,out]    array       Pointer to an array of longs to be gathered/sent
* @param[in]        sizes       Array containing the number of longs to be gathered from each process
* @param[in]        displs      Array of displacements
* @param[in]        root        Rank of the root process
*/
void DAL_l_gatherv( const TestInfo *ti, long **array, int *sizes, int *displs, int root )
{
    if( GET_ID( ti ) == root ) {
        return DAL_l_gathervReceive( ti, array, sizes, displs );
    }
    else {
        return DAL_l_gathervSend( ti, array, sizes[0], root );
    }
}






/**
* @brief Sends data from all to all processes
*
* @param[in]        ti          The test info
* @param[in,out]    data        Data to be sent/received
* @param[in]        size        Number of longs to be sent/received to/from each process
*/
void DAL_l_alltoall( const TestInfo *ti, long **array, int size )
{
    long *recvArray = (long*)malloc( size * GET_N( ti ) * sizeof(long) );
    assert( recvArray != NULL );

    MPI_Alltoall( *array, size, MPI_LONG, recvArray, size, MPI_LONG, MPI_COMM_WORLD );

    free( *array );
    *array = recvArray;
}







/**
* @brief Sends array from all to all processes
*
* @param[in]        ti          The test info
* @param[in,out]    array       Pointer to an array of longs to be sent/received
* @param[in]        sendSizes   Array containing the number of longs to be sent to each process
* @param[in]        sdispls     Array of displacements
* @param[in]        recvSizes   Array containing the number of longs to be received from each process
* @param[in]        rdispls     Array of displacements
*/
void DAL_l_alltoallv( const TestInfo *ti, long **array, int *sendSizes, int *sdispls, int *recvSizes, int *rdispls )
{
    int rcount = 0;
    int i;

    for ( i=0; i<GET_N( ti ); i++ ) {
        rcount += recvSizes[i];
    }
    long *recvArray = (long*)malloc( rcount*sizeof(long) );
    assert( recvArray != NULL );

    MPI_Alltoallv( *array, sendSizes, sdispls, MPI_LONG, recvArray, recvSizes, rdispls, MPI_LONG, MPI_COMM_WORLD );

    free( *array );
    *array = recvArray;
}






void DAL_l_bcastSend( const TestInfo *ti, long **array, int size )
{
    MPI_Bcast( *array, size, MPI_LONG, GET_ID( ti ), MPI_COMM_WORLD );
}
void DAL_l_bcastReceive( const TestInfo *ti, long **array, int size, int root )
{
    *array = (long*)malloc( size * sizeof(long) );
    assert( *array != NULL );
    MPI_Bcast( *array, size, MPI_LONG, root, MPI_COMM_WORLD );
}
/**
* @brief Broadcasts array to processes
*
* @param[in] ti         The test info
* @param[in] array      Pointer to an array of longs to be broadcast
* @param[in] size       Number of longs per process
* @param[in] root       Rank of the root process
*/
void DAL_l_bcast( const TestInfo *ti, long **array, int size, int root )
{
    if( GET_ID( ti ) == root ) {
        return DAL_l_bcastSend( ti, array, size );
    }
    else {
        return DAL_l_bcastReceive( ti, array, size, root );
    }
}

/*--------------------------------------------------------------------------------------------------------------*/
