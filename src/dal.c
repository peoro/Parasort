
#include <errno.h>
#include <mpi.h>
#include "dal.h"



static inline int GET_ID ( ) {
    int x;
    MPI_Comm_rank ( MPI_COMM_WORLD, &x );
    return x;
}
static inline int GET_N ( ) {
    int x;
    MPI_Comm_size ( MPI_COMM_WORLD, &x );
    return x;
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
#define FILE_READ( dst, size, file )	fread( dst, sizeof(int), size, file ) // reads size items (integers!)
#define FILE_WRITE( src, size, file )	fwrite( src, sizeof(int), size, file ) // writes size items (integers!)

const char *DAL_mediumName( DataMedium m )
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
			snprintf( s, size, "\"%s\" [file on disk] of %ld bytes", d->file.name, DAL_dataSize(d) );
			break;
		case Array:
			snprintf( s, size, "array [on principal memory @ %p] of %ld bytes", d->array.data, DAL_dataSize(d) );
			break;
		default:
			snprintf( s, size, "Unknow medium code %d [%s]", d->medium, DAL_mediumName(d->medium) );
	}
	return s;
}
char * DAL_dataItemsToString( Data *d, char *s, int size )
{
	char buf[64];
	int i;

	s[0] = '\0';

	if( ! size ) {
		return s;
	}
	if( ! d->array.size ) {
		strncat( s, "{}", size );
		return s;
	}

	strncat( s, "{", size );
	for( i = 0; i < d->array.size-1; ++ i ) {
		snprintf( buf, sizeof(buf), "%d,", d->array.data[i] );
		strncat( s, buf, size );
	}

	snprintf( buf, sizeof(buf), "%d}", d->array.data[i] );
	strncat( s, buf, size );

	if( strlen(s) == (unsigned) size-1 ) {
		s[size-4] = s[size-3] = s[size-2] = '.';
		s[size-1] = '}';
	}

	return s;
}



/***************************************************************************************************************/
/******************************************** DAL Data management **********************************************/
/***************************************************************************************************************/

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
			fclose( data->file.handle );
			if( ! remove(data->file.name) ) {
				SPD_DEBUG( "Error removing data file \"%s\"", data->file.name );
			}
			break;
		}
		case Array: {
			free( data->array.data );
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}
	
	DAL_init( data );
}
long DAL_dataSize( Data *data )
{
	switch( data->medium ) {
		case File: {
			long len;
			long cursor;
			cursor = ftell( data->file.handle );          // getting current position
			fseek( data->file.handle, 0, SEEK_END );      // going to end
			len = ftell( data->file.handle );             // getting current position
			fseek( data->file.handle, cursor, SEEK_SET ); // getting back to where I was
			return len / sizeof(int);
			break;
		}
		case Array: {
			return data->array.size;
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}
}
bool DAL_allocData( Data *data, long size )
{
	DAL_ASSERT( DAL_isInitialized(data), data, "data should have been initialized" );
	
	if( 0 && DAL_allocArray(data, size) ) {
		return 1;
	}
	
	if( DAL_allocFile(data, size) ) {
		return 1;
	}
	
	return 0;
}
bool DAL_readFile( Data *data, const char *path )
{
	long size = GET_FILE_SIZE(path) / sizeof(int);
	DAL_allocData( data, size );
	
	// temporary, just to use DAL_readNextDeviceBlock
	// won't destroy it, or it would remove path!
	Data dataStub;
	dataStub.medium = File;
	strncpy( dataStub.file.name, path, sizeof(dataStub.file.name) );
	dataStub.file.handle = fopen( path, "r" );
	if( ! dataStub.file.handle ) {
		SPD_DEBUG( "Cannot open \"%s\" for reading.", path );
		return 0;
	}
	
	DAL_readNextDeviceBlock( &dataStub, data );
	
	fclose( dataStub.file.handle );
	return 1;
}
bool DAL_writeFile( Data *data, const char *path )
{
	long size = GET_FILE_SIZE(path) / sizeof(int);
	
	// temporary, just to use DAL_writeNextDeviceBlock
	// won't destroy it, or it would remove path!
	Data dataStub;
	dataStub.medium = File;
	strncpy( dataStub.file.name, path, sizeof(dataStub.file.name) );
	dataStub.file.handle = fopen( path, "w" );
	if( ! dataStub.file.handle ) {
		SPD_DEBUG( "Cannot open \"%s\" for writing.", path );
		return 0;
	}
	
	DAL_writeNextDeviceBlock( &dataStub, data );
	
	fclose( dataStub.file.handle );
	return 1;
}

// functions to work with any find of block device (ie: Files)
bool DAL_isDevice( Data *data )
{
	return data->medium == File;
}
void DAL_resetDeviceCursor( Data *device )
{
	DAL_ASSERT( DAL_isDevice(device), device, "not a block/character device" );
	
	if( device->medium == File ) {
		rewind( device->file.handle );
	}
	else {
		DAL_UNIMPLEMENTED( device );
	}
}
void DAL_readNextDeviceBlock( Data *device, Data *dst )
{
	DAL_ASSERT( DAL_isDevice(device), device, "not a block/character device" );
	
	// DAL_DEBUG( device, "reading from this device" );
	// DAL_DEBUG( dst, "into this data" );
	
	switch( dst->medium ) {
		case File: {
			Data buffer;
			DAL_init( &buffer );
			SPD_ASSERT( DAL_allocBuffer( &buffer, DAL_dataSize(device) ), "memory completely over..." );
			const long bufSize = DAL_dataSize(&buffer);
			long r = 0;
			long read = 0;
			long total = 0;
			while( total != DAL_dataSize(device) ) {
				r = FILE_READ( buffer.array.data, bufSize, device->file.handle );
				FILE_WRITE( buffer.array.data, r, dst->file.handle );
				total += r;
			}
			break;
		}
		case Array: {
			FILE_READ( dst->array.data, dst->array.size, device->file.handle );
			break;
		}
		default:
			DAL_UNSUPPORTED( dst );
	}
}
void DAL_writeNextDeviceBlock( Data *device, Data *src )
{
	DAL_ASSERT( DAL_isDevice(device), device, "not a block/character device" );
	
	//DAL_DEBUG( device, "writing into this device" );
	//DAL_DEBUG( src, "from this data" );
	
	if( src->medium == Array ) {
		FILE_WRITE( src->array.data, src->array.size, device->file.handle );
	}
	else {
		DAL_UNIMPLEMENTED( src );
	}
}

// allocating an Array in memory
bool DAL_allocArray( Data *data, long size )
{
	DAL_ASSERT( DAL_isInitialized(data), data, "data should have been initialized" );

	if( size == 0 ) {
		data->medium = Array;
		data->array.data = 0;
		data->array.size = 0;
		return 1;
	}
	
	// TODO: maybe add a threshold on max size
	// if( size > threshold ) { return 0; }

	data->array.data = (int*) malloc( size * sizeof(int) );
	if( ! data->array.data ) {
		return 0;
	}
	data->array.size = size;

	data->medium = Array;

	return 1;
}
bool DAL_reallocArray ( Data *data, long size )
{
	DAL_ASSERT( data->medium == Array, data, "only Array data can be reallocated" );

	if( size == 0 ) {
		data->medium = Array;
		free( data->array.data );
		data->array.data = 0;
		data->array.size = 0;
		return 1;
	}

	data->array.data = (int*) realloc( data->array.data, size * sizeof(int) );
	if( ! data->array.data ) {
		return 0;
	}
	data->array.size = size;

	data->medium = Array;

	return 1;
}
bool DAL_reallocAsArray( Data *data )
{
	DAL_ASSERT( DAL_isInitialized(data), data, "data shouldn't have been initialized" );
	
	if( data->medium == Array ) {
		SPD_WARNING( "tring to reallocate an Array as an Array..." );
		return 1;
	}
	
	Data arrayData;
	DAL_init( &arrayData );
	if( ! DAL_allocArray( &arrayData, data->array.size ) ) {
		return 0;
	}
	
	// OK, array created, moving actual data into it
	if( data->medium == File ) {
		DAL_resetDeviceCursor( data );
		DAL_readNextDeviceBlock( data, &arrayData );
		return 0;
	}
	else {
		DAL_UNIMPLEMENTED( data );
	}
	
	// destroying old data, and replacing it with new one...
	DAL_destroy( data );
	*data = arrayData;
	
	return 1;
}

// allocating a temporary buffer in memory (an Array)
bool DAL_allocBuffer( Data *data, long size )
{
	DAL_ASSERT( DAL_isInitialized(data), data, "data should have been initialized" );
	
	if( size == 0 ) {
		return 0;
	}
	
	// TODO: implement a better logic:
	//       start from DAL_allocArray's threshold
	//       (or a well-chosen value) and shrinks according to some heuristic...
	
	bool r =  DAL_allocArray( data, size );
	while( size > 0 && ! r ) {
		size /= 2;
		r =  DAL_allocArray( data, size );
	}
	
	return r;
}
// allocating a File
static inline char toFileChar( int n )
{
	const int chars = 'Z'-'A'+1, nums = '9'-'0'+1;
	// check this function validity...
	n %= ( chars + nums );
	if( n < nums ) {
		return n+'0';
	}
	else {
		n -= nums;
		return n+'A';
	}
}
bool DAL_allocFile( Data *data, long size )
{
	DAL_ASSERT( DAL_isInitialized(data), data, "data should have been initialized" );
	
	FILE *handle;
	char name[128];
	do {
		#define X ( toFileChar(rand()) )
		#define X4 X,X,X,X
		snprintf( name, sizeof(name), "tmp%d_%c%c%c%c-%c%c%c%c-%c%c%c%c-%c%c%c%c", GET_N(), X4, X4, X4, X4 );
		#undef X4
		#undef X
		handle = fopen( name, "r" );
	} while( handle ); // if file exists, try again!
	
	handle = fopen( name, "w+" );
	if( ! handle ) {
		SPD_DEBUG( "File \"%s\" couldn't be opened: %s", name, strerror(errno) );
	}
	
	data->medium = File;
	strncpy( data->file.name, name, sizeof(data->file.name) );
	data->file.handle = handle;
	
	return 1;
}




/***************************************************************************************************************/
/************************************* [Data] Communication Primitives *****************************************/
/***************************************************************************************************************/
// tiny wrappers for MPI
// these don't use Datas, memory arrays like MPI do, but are so much more comfortable for our purpose...
static inline DAL_MPI_SEND( char *array, long size, int dest, int dataType ) {
	MPI_Send( array, size, dataType, dest, 0, MPI_COMM_WORLD );
}
static inline DAL_MPI_RECEIVE( char *array, long size, int source, int dataType ) {
	MPI_Status 	stat;
	MPI_Recv( array, size, dataType, source, 0, MPI_COMM_WORLD, &stat );
}

/**
* @brief Sends data to dest
*
* @param[in] ti       	The test info
* @param[in] data  		Data to be sent
* @param[in] dest     	Rank of the receiver process
*/
void DAL_send( Data *data, int dest )
{
	switch( data->medium ) {
		case File: {
			DAL_UNIMPLEMENTED( data );
			break;
		}
		case Array: {
			char buf[64];
				//SPD_DEBUG( "sending %ld items to %d: %s", data->array.size, dest, DAL_dataItemsToString(data,buf,sizeof(buf)) );
			DAL_MPI_SEND( data->array.data, data->array.size, dest, MPI_INT );
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
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
void DAL_receive( Data *data, long size, int source )
{
	char buf[64];
	SPD_ASSERT( DAL_allocArray( data, size ), "not enough memory to allocate data" );
		//SPD_DEBUG( "receiving %ld items from %d", size, source );
	DAL_MPI_RECEIVE( data->array.data, size, source, MPI_INT );
		//SPD_DEBUG( "received items: %s", DAL_dataItemsToString(data,buf,sizeof(buf)) );
}

void DAL_sendU( Data *data, int dest )
{
	//SPD_DEBUG( "telling I'll be sending %ld items", data->array.size );
	DAL_MPI_SEND( & data->array.size, 1, dest, MPI_LONG ); // sending size
	DAL_send( data, dest ); // sending data
}
void DAL_receiveU( Data *data, int source )
{
	long size;
	DAL_MPI_RECEIVE( &size, 1, source, MPI_LONG ); // receiving size
	//SPD_DEBUG( "heard I'll be receiving %ld items", size );
	DAL_receive( data, size, source ); // receiving data
}
void DAL_receiveA( Data *data, long size, int source )
{
	long int oldDataSize = data->array.size;
		//SPD_DEBUG( "appending %ld items to the %ld I've already got: %s", size, oldDataSize, DAL_dataItemsToString(data,buf,sizeof(buf)) );
	SPD_ASSERT( DAL_reallocArray( data, data->array.size + size ), "not enough memory to allocate data" );
	DAL_MPI_RECEIVE( data->array.data + oldDataSize, size, source, MPI_INT );
		//SPD_DEBUG( "post-appending items: %s", DAL_dataItemsToString(data,buf,sizeof(buf)) );
}
void DAL_receiveAU( Data *data, int source )
{
	long size;
	DAL_MPI_RECEIVE( &size, 1, source, MPI_LONG ); // receiving size
	//SPD_DEBUG( "heard I'll be appending %ld items to the %ld I've already got", size, data->array.size );
	DAL_receiveA( data, size, source ); // receiving data
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
long DAL_sendrecv( Data *sdata, long scount, long sdispl, Data* rdata, long rcount, long rdispl, int partner )
{
	int recvCount = rcount;
	int sendCount = scount;
	int recvDispl = rdispl;
	int sendDispl = sdispl;

	if( rdata->medium == NoMedium ) {
		SPD_ASSERT( DAL_allocArray( rdata, recvDispl+recvCount ), "not enough memory to allocate data" );
	}
	else {
		SPD_ASSERT( DAL_reallocArray( rdata, recvDispl+recvCount ), "not enough memory to allocate data" );
	}

	MPI_Status 	status;
	MPI_Sendrecv( sdata->array.data+sendDispl, sendCount, MPI_INT, partner, 100, rdata->array.data+recvDispl, recvCount, MPI_INT, partner, 100, MPI_COMM_WORLD, &status );

	MPI_Get_count( &status, MPI_INT, &recvCount );
	SPD_ASSERT( DAL_reallocArray( rdata, recvDispl+recvCount ), "not enough memory to allocate data" );

	rcount = recvCount;
	return rcount;
}






void DAL_scatterSend( Data *data )
{
	switch( data->medium ) {
		case File: {
			DAL_UNIMPLEMENTED( data );
			break;
		}
		case Array: {
			DAL_ASSERT( data->array.size % GET_N() == 0, data, "data should be a multiple of %d (but it's %ld)", GET_N(), data->array.size );
			MPI_Scatter( data->array.data, data->array.size/GET_N(), MPI_INT, MPI_IN_PLACE, data->array.size/GET_N(), MPI_INT, GET_ID(), MPI_COMM_WORLD );
			SPD_ASSERT( DAL_reallocArray( data, data->array.size/GET_N() ), "not enough memory to allocate data" );
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}
}
void DAL_scatterReceive( Data *data, long size, int root )
{
	SPD_ASSERT( DAL_allocArray( data, size ), "not enough memory to allocate data" );
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
void DAL_scatter( Data *data, long size, int root )
{
	if( GET_ID() == root ) {
		return DAL_scatterSend( data );
	}
	else {
		return DAL_scatterReceive( data, size, root );
	}
}





void DAL_gatherSend( Data *data, int root )
{
	switch( data->medium ) {
		case File: {
			DAL_UNIMPLEMENTED( data );
			break;
		}
		case Array: {
			MPI_Gather( data->array.data, data->array.size, MPI_INT, NULL, 0, MPI_INT, root, MPI_COMM_WORLD );
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}
}
void DAL_gatherReceive( Data *data, long size )
{
	SPD_ASSERT( size % GET_N() == 0, "size should be a multiple of %d (but it's %ld)", GET_N(), size );
	SPD_ASSERT( DAL_reallocArray( data, size ), "not enough memory to allocate data" );
	MPI_Gather( MPI_IN_PLACE, size/GET_N(), MPI_INT, data->array.data, size/GET_N(), MPI_INT, GET_ID(), MPI_COMM_WORLD );
}
/**
* @brief Gathers data from all processes
*
* @param[in] 		ti       	The test info
* @param[in,out] 	data  		Data to be gathered/sent
* @param[in] 		size     	Number of elements to be gathered
* @param[in] 		root     	Rank of the root process
*/
void DAL_gather( Data *data, long size, int root )
{
	if( GET_ID() == root ) {
		return DAL_gatherReceive( data, size );
	}
	else {
		return DAL_gatherSend( data, root );
	}
}







void DAL_scattervSend( Data *data, long *sizes, long *displs )
{
	switch( data->medium ) {
		case File: {
			DAL_UNIMPLEMENTED( data );
			break;
		}
		case Array: {
			int scounts[GET_N()];
			int sdispls[GET_N()];
			int i;

			for ( i=0; i<GET_N(); i++ ) {
				scounts[i] = sizes[i];
				sdispls[i] = displs[i];
			}
		 	MPI_Scatterv( data->array.data, scounts, sdispls, MPI_INT, MPI_IN_PLACE, scounts[GET_ID()], MPI_INT, GET_ID(), MPI_COMM_WORLD );
			SPD_ASSERT( DAL_reallocArray( data, scounts[GET_ID()] ), "not enough memory to allocate data" );
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}
}
void DAL_scattervReceive( Data *data, long size, int root )
{
	SPD_ASSERT( DAL_allocArray( data, size ), "not enough memory to allocate data" );
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
void DAL_scatterv( Data *data, long *sizes, long *displs, int root )
{
	if( GET_ID() == root ) {
		return DAL_scattervSend( data, sizes, displs );
	}
	else {
		return DAL_scattervReceive( data, sizes[GET_ID()], root );
	}
}




void DAL_gathervSend( Data *data, int root )
{
	switch( data->medium ) {
		case File: {
			DAL_UNIMPLEMENTED( data );
			break;
		}
		case Array: {
			MPI_Gatherv( data->array.data, data->array.size, MPI_INT, NULL, NULL, NULL, MPI_INT, root, MPI_COMM_WORLD );
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}
}
void DAL_gathervReceive( Data *data, long *sizes, long *displs )
{
	int rcounts[GET_N()];
	int rdispls[GET_N()];
	int rcount = 0;
	int i;

	for ( i=0; i<GET_N(); i++ ) {
		rcounts[i] = sizes[i];
		rdispls[i] = displs[i];

		rcount += sizes[i];
	}
	SPD_ASSERT( DAL_reallocArray( data, rcount ), "not enough memory to allocate data" );
	MPI_Gatherv( MPI_IN_PLACE, rcounts[GET_ID()], MPI_INT, data->array.data, rcounts, rdispls, MPI_INT, GET_ID(), MPI_COMM_WORLD );
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
void DAL_gatherv( Data *data, long *sizes, long *displs, int root )
{
	if( GET_ID() == root ) {
		return DAL_gathervReceive( data, sizes, displs );
	}
	else {
		return DAL_gathervSend( data, root );
	}
}







/**
* @brief Sends data from all to all processes
*
* @param[in] 		ti       	The test info
* @param[in,out] 	data  		Data to be sent/received
* @param[in] 		size  		Number of elements to be sent/received to/from each process
*/
void DAL_alltoall( Data *data, long size )
{
// 	DAL_UNSUPPORTED( data );
	int count = size;
	int i;

	Data recvData;
	DAL_init( &recvData );
	SPD_ASSERT( DAL_allocArray(&recvData, count), "not enough memory to allocate data" );

	MPI_Alltoall( data->array.data, count, MPI_INT, recvData.array.data, count, MPI_INT, MPI_COMM_WORLD );

	DAL_destroy( data );
	*data = recvData;
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
void DAL_alltoallv( Data *data, long *sendSizes, long *sdispls, long *recvSizes, long *rdispls )
{
// 	DAL_UNSUPPORTED( data );

	int scounts[GET_N()];
	int sd[GET_N()];
	int rcounts[GET_N()];
	int rd[GET_N()];
	int rcount = 0;
	int i;

	for ( i=0; i<GET_N(); i++ ) {
		scounts[i] = sendSizes[i];
		sd[i] = sdispls[i];

		rcounts[i] = recvSizes[i];
		rd[i] = rdispls[i];

		rcount += rcounts[i];
	}
	Data recvData;
	DAL_init( &recvData );
	SPD_ASSERT( DAL_allocArray(&recvData, rcount), "not enough memory to allocate data" );

	MPI_Alltoallv( data->array.data, scounts, sd, MPI_INT, recvData.array.data, rcounts, rd, MPI_INT, MPI_COMM_WORLD );

	DAL_destroy( data );
	*data = recvData;
}





void DAL_bcastSend( Data *data )
{
	switch( data->medium ) {
		case File: {
			DAL_UNIMPLEMENTED( data );
			break;
		}
		case Array: {
			MPI_Bcast( data->array.data, data->array.size, MPI_INT, GET_ID(), MPI_COMM_WORLD );
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}
}
void DAL_bcastReceive( Data *data, long size, int root )
{
	SPD_ASSERT( DAL_allocArray( data, size ), "not enough memory to allocate data" );
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
void DAL_bcast( Data *data, long size, int root )
{
	if( GET_ID() == root ) {
		return DAL_bcastSend( data );
	}
	else {
		return DAL_bcastReceive( data, size, root );
	}
}
/*--------------------------------------------------------------------------------------------------------------*/

