
#include<assert.h>
#include <mpi.h>
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


int DAL_getTypeMPI( DAL_Type type ) {
	switch ( type ) {
        case DAL_CHAR: return MPI_CHAR;
        case DAL_SIGNED_CHAR: return MPI_SIGNED_CHAR;
        case DAL_UNSIGNED_CHAR: return MPI_UNSIGNED_CHAR;
        case DAL_BYTE: return MPI_BYTE;
        case DAL_WCHAR: return MPI_WCHAR;
        case DAL_SHORT: return MPI_SHORT;
        case DAL_UNSIGNED_SHORT: return MPI_UNSIGNED_SHORT;
        case DAL_INT: return MPI_INT;
        case DAL_UNSIGNED: return MPI_UNSIGNED;
        case DAL_LONG: return MPI_LONG;
        case DAL_UNSIGNED_LONG: return MPI_UNSIGNED_LONG;
        case DAL_FLOAT: return MPI_FLOAT;
        case DAL_DOUBLE: return MPI_DOUBLE;
        case DAL_LONG_DOUBLE: return MPI_LONG_DOUBLE;
        case DAL_LONG_LONG_INT: return MPI_LONG_LONG_INT;
        case DAL_LONG_LONG: return MPI_LONG_LONG;
        case DAL_UNSIGNED_LONG_LONG: return MPI_UNSIGNED_LONG_LONG;
		default: return MPI_INT;
	}
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
* @param[in] size  		Max number of elements to be received
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
* @param[in] scount		Number of elements to be sent
* @param[in] sdispl		Displacement for the send buffer
* @param[in] rdata		Data buffer to store received elements
* @param[in] rcount		Max number of elements to be received
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
* @param[in] size     	Number of elements per process
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
* @param[in] 		size     	Number of elements to be gathered
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
* @param[in] 		sizes     	Array containing the number of elements to be sent to each process
* @param[in] 		displs     	Array of displacements
* @param[in] 		root     	Rank of the root process
*/
void DAL_scatterv( const TestInfo *ti, Data *data, long *sizes, long *displs, int root )
{
	if( GET_ID( ti ) == root ) {
		return DAL_scattervSend( ti, data, sizes, displs );
	}
	else {
		return DAL_scattervReceive( ti, data, sizes[0], root );
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
	int rcount = 0;
	int i;

	for ( i=0; i<GET_N( ti ); i++ ) {
		rcounts[i] = sizes[i];
		rdispls[i] = displs[i];

		rcount += sizes[i];
	}
	assert( DAL_reallocArray( data, rcount ) );
	MPI_Gatherv( MPI_IN_PLACE, rcounts[GET_ID( ti )], MPI_INT, data->array.data, rcounts, rdispls, MPI_INT, GET_ID( ti ), MPI_COMM_WORLD );
}
/**
* @brief Gathers data from all processes
*
* @param[in] 		ti       	The test info
* @param[in,out] 	data  		Data to be gathered/sent
* @param[in] 		sizes     	Array containing the number of elements to be gathered from each process
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
* @param[in] 		size  		Number of elements to be sent/received to/from each process
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
* @param[in] 		sendSizes  	Array containing the number of elements to be sent to each process
* @param[in] 		sdispls     Array of displacements
* @param[in] 		recvSizes  	Array containing the number of elements to be received from each process
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
* @param[in] size     	Number of elements per process
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
/********************************* [DAL_Type] Communication Primitives *****************************************/
/***************************************************************************************************************/

/**
* @brief Sends array to dest
*
* @param[in] ti         The test info
* @param[in] array      Array of elements to be sent
* @param[in] size       Length of the array
* @param[in] type       Type of the array elements
* @param[in] dest       Rank of the receiver process
*/
void DAL_type_send( const TestInfo *ti, void *array, int size, DAL_Type type, int dest )
{
    MPI_Send( array, size, DAL_getTypeMPI( type ), dest, 0, MPI_COMM_WORLD );
}

/**
* @brief Receives array from source
*
* @param[in] ti         The test info
* @param[in] array      Array buffer to store received elements
* @param[in] size       Max number of elements to be received
* @param[in] type       Type of the array elements
* @param[in] source     Rank of the sender process
*
* @returns Number of received elements
*/
int DAL_type_receive( const TestInfo *ti, void *array, int size, DAL_Type type, int source )
{
    MPI_Status  status;

    assert( array != NULL );

    MPI_Recv( array, size, DAL_getTypeMPI( type ), source, 0, MPI_COMM_WORLD, &status );

    MPI_Get_count( &status, DAL_getTypeMPI( type ), &size );

    return size;
}





/**
* @brief Sends and receives array from partner
*
* @param[in] ti         The test info
* @param[in] sarray     Array of elements to be sent
* @param[in] scount     Number of elements to be sent
* @param[in] rarray     Buffer to store received elements
* @param[in] rcount     Max number of elements to be received
* @param[in] type       Type of the array elements
* @param[in] partner    Rank of the partner process
*
* @returns Number of received elements
*/
int DAL_type_sendrecv( const TestInfo *ti, void *sarray, int scount, void *rarray, int rcount, DAL_Type type, int partner )
{
    assert( sarray != NULL );
    assert( rarray != NULL );

    MPI_Status  status;
    MPI_Sendrecv( sarray, scount, DAL_getTypeMPI( type ), partner, 100, rarray, rcount, DAL_getTypeMPI( type ), partner, 100, MPI_COMM_WORLD, &status );

    MPI_Get_count( &status, DAL_getTypeMPI( type ), &rcount );

    return rcount;
}







void DAL_type_scatterSend( const TestInfo *ti, void *array, int size, DAL_Type type )
{
    assert( size % GET_N( ti ) == 0 );
    MPI_Scatter( array, size, DAL_getTypeMPI( type ), MPI_IN_PLACE, size, DAL_getTypeMPI( type ), GET_ID( ti ), MPI_COMM_WORLD );
}
void DAL_type_scatterReceive( const TestInfo *ti, void *array, int size, DAL_Type type, int root )
{
    MPI_Scatter( NULL, 0, DAL_getTypeMPI( type ), array, size, DAL_getTypeMPI( type ), root, MPI_COMM_WORLD );
}
/**
* @brief Scatters array among all processes
*
* @param[in] ti         The test info
* @param[in] array      Array of elements to be scattered
* @param[in] size       Number of elements per process
* @param[in] type       Type of the array elements
* @param[in] root       Rank of the root process
*/
void DAL_type_scatter( const TestInfo *ti, void *array, int size, DAL_Type type, int root )
{
    assert( array != NULL );

    if( GET_ID( ti ) == root ) {
        return DAL_type_scatterSend( ti, array, size, type );
    }
    else {
        return DAL_type_scatterReceive( ti, array, size, type, root );
    }
}







void DAL_type_gatherSend( const TestInfo *ti, void *array, int size, DAL_Type type, int root )
{
    MPI_Gather( array, size, DAL_getTypeMPI( type ), NULL, 0, DAL_getTypeMPI( type ), root, MPI_COMM_WORLD );
}
void DAL_type_gatherReceive( const TestInfo *ti, void *array, int size, DAL_Type type )
{
    assert( size % GET_N( ti ) == 0 );
    MPI_Gather( MPI_IN_PLACE, size/GET_N( ti ), DAL_getTypeMPI( type ), array, size/GET_N( ti ), DAL_getTypeMPI( type ), GET_ID( ti ), MPI_COMM_WORLD );
}
/**
* @brief Gathers array from all processes
*
* @param[in]        ti          The test info
* @param[in,out]    array       Array of elements to be gathered/sent
* @param[in]        size        Number of elements to be gathered
* @param[in]        type       Type of the array elements
* @param[in]        root        Rank of the root process
*/
void DAL_type_gather( const TestInfo *ti, void *array, int size, DAL_Type type, int root )
{
    assert( array != NULL );

    if( GET_ID( ti ) == root ) {
        return DAL_type_gatherReceive( ti, array, size, type );
    }
    else {
        return DAL_type_gatherSend( ti, array, size/GET_N( ti ), type, root );
    }
}








void DAL_type_scattervSend( const TestInfo *ti, void *array, int *sizes, int *displs, DAL_Type type )
{
    MPI_Scatterv( array, sizes, displs, DAL_getTypeMPI( type ), MPI_IN_PLACE, sizes[0], DAL_getTypeMPI( type ), GET_ID( ti ), MPI_COMM_WORLD );
}
void DAL_type_scattervReceive( const TestInfo *ti, void *array, int size, DAL_Type type, int root )
{
    MPI_Scatterv( NULL, NULL, NULL, DAL_getTypeMPI( type ), array, size, DAL_getTypeMPI( type ), root, MPI_COMM_WORLD );
}
/**
* @brief Scatters array among all processes
*
* @param[in]        ti          The test info
* @param[in,out]    array       Array of elements to be scattered/received
* @param[in]        sizes       Array containing the number of elements to be sent to each process
* @param[in]        displs      Array of displacements
* @param[in]        type       Type of the array elements
* @param[in]        root        Rank of the root process
*/
void DAL_type_scatterv( const TestInfo *ti, void *array, int *sizes, int *displs, DAL_Type type, int root )
{
    assert( array != NULL );

     if( GET_ID( ti ) == root ) {
        return DAL_type_scattervSend( ti, array, sizes, displs, type );
    }
    else {
        return DAL_type_scattervReceive( ti, array, sizes[0], type, root );
    }
}








void DAL_type_gathervSend( const TestInfo *ti, void *array, int size, DAL_Type type, int root )
{
    MPI_Gatherv( array, size, DAL_getTypeMPI( type ), NULL, NULL, NULL, DAL_getTypeMPI( type ), root, MPI_COMM_WORLD );
}
void DAL_type_gathervReceive( const TestInfo *ti, void *array, int *sizes, int *displs, DAL_Type type )
{
    int rcount = 0;
    int i;

    for ( i=0; i<GET_N( ti ); i++ ) {
        rcount += sizes[i];
    }
    MPI_Gatherv( MPI_IN_PLACE, sizes[GET_ID( ti )], DAL_getTypeMPI( type ), array, sizes, displs, DAL_getTypeMPI( type ), GET_ID( ti ), MPI_COMM_WORLD );
}
/**
* @brief Gathers array from all processes
*
* @param[in]        ti          The test info
* @param[in,out]    array       Array of elements to be gathered/sent
* @param[in]        sizes       Array containing the number of elements to be gathered from each process
* @param[in]        displs      Array of displacements
* @param[in]        type       Type of the array elements
* @param[in]        root        Rank of the root process
*/
void DAL_type_gatherv( const TestInfo *ti, void *array, int *sizes, int *displs, DAL_Type type, int root )
{
    assert( array != NULL );

    if( GET_ID( ti ) == root ) {
        return DAL_type_gathervReceive( ti, array, sizes, displs, type );
    }
    else {
        return DAL_type_gathervSend( ti, array, sizes[0], type, root );
    }
}






/**
* @brief Sends data from all to all processes
*
* @param[in]        ti          The test info
* @param[in,out]    sarray        Data to be sent
* @param[in]        rarray      Array of elements to be received
* @param[in]        size        Number of elements to be sent/received to/from each process
* @param[in]        type        Type of the array elements
*/
void DAL_type_alltoall( const TestInfo *ti, void *sarray, void *rarray, int size, DAL_Type type )
{
    assert( sarray != NULL );
    assert( rarray != NULL );
    MPI_Alltoall( sarray, size, DAL_getTypeMPI( type ), rarray, size, DAL_getTypeMPI( type ), MPI_COMM_WORLD );
}







/**
* @brief Sends array from all to all processes
*
* @param[in]        ti          The test info
* @param[in]        sarray      Array of elements to be sent
* @param[in]        sendSizes   Array containing the number of elements to be sent to each process
* @param[in]        sdispls     Array of displacements
* @param[in]        rarray      Array of elements to be received
* @param[in]        recvSizes   Array containing the number of elements to be received from each process
* @param[in]        rdispls     Array of displacements
* @param[in]        type        Type of the array elements
*/
void DAL_type_alltoallv( const TestInfo *ti, void *sarray, int *sendSizes, int *sdispls, void *rarray, int *recvSizes, int *rdispls, DAL_Type type )
{
    int rcount = 0;
    int i;

    for ( i=0; i<GET_N( ti ); i++ ) {
        rcount += recvSizes[i];
    }
    assert( sarray != NULL );
    assert( rarray != NULL );
    MPI_Alltoallv( sarray, sendSizes, sdispls, DAL_getTypeMPI( type ), rarray, recvSizes, rdispls, DAL_getTypeMPI( type ), MPI_COMM_WORLD );
}






void DAL_type_bcastSend( const TestInfo *ti, void *array, int size, DAL_Type type )
{
    MPI_Bcast( array, size, DAL_getTypeMPI( type ), GET_ID( ti ), MPI_COMM_WORLD );
}
void DAL_type_bcastReceive( const TestInfo *ti, void *array, int size, DAL_Type type, int root )
{
    MPI_Bcast( array, size, DAL_getTypeMPI( type ), root, MPI_COMM_WORLD );
}
/**
* @brief Broadcasts array to processes
*
* @param[in] ti         The test info
* @param[in] array      Array of elements to be broadcast
* @param[in] size       Number of elements per process
* @param[in] type       Type of the array elements
* @param[in] root       Rank of the root process
*/
void DAL_type_bcast( const TestInfo *ti, void *array, int size, DAL_Type type, int root )
{
    assert( array != NULL );

    if( GET_ID( ti ) == root ) {
        return DAL_type_bcastSend( ti, array, size, type );
    }
    else {
        return DAL_type_bcastReceive( ti, array, size, type, root );
    }
}

/*--------------------------------------------------------------------------------------------------------------*/
