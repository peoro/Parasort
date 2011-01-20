
#include <errno.h>
#include <mpi.h>
#include "dal.h"
#include "dal_internals.h"


#define MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define MAX(a,b) ( (a)>(b) ? (a) : (b) )


// a global variable to implement DAL functions (send and receive)
// with the static solution.
// is initialized by DAL_initialize
// and destroyed by DAL_finalize
Data DAL_buffer;
bool DAL_bufferAcquired = 0;

void DAL_acquireGlobalBuffer( Data *data )
{
	SPD_ASSERT( ! DAL_bufferAcquired, "global buffer already acquired..." );
	*data = DAL_buffer;
	DAL_bufferAcquired = 1;
}
void DAL_releaseGlobalBuffer( Data *data )
{
	SPD_ASSERT( DAL_bufferAcquired, "global buffer not acquired..." );
	data->medium = NoMedium;
	DAL_bufferAcquired = 0;
}


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



dal_size_t GET_FILE_SIZE( const char *path )
{
	FILE *f;
	dal_size_t len;

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
			snprintf( s, size, "\"%s\" [file on disk] of %lld items", d->file.name, DAL_dataSize(d) );
			break;
		case Array:
			snprintf( s, size, "array [on principal memory @ %p] of %lld items", d->array.data, DAL_dataSize(d) );
			break;
		default:
			snprintf( s, size, "Unknow medium code %d [%s]", d->medium, DAL_mediumName(d->medium) );
	}
	return s;
}
char * DAL_dataItemsToString( Data *data, char *s, int size )
{
	if( data->medium == File ) {
		Data b;

		DAL_init( &b );

		if( DAL_dataSize(data) ) {
			SPD_ASSERT( DAL_allocBuffer( &b, DAL_dataSize(data) ), "memory completely over..." );
			DAL_dataCopyO( data, 0, &b, 0 );
		}
		else {
			SPD_ASSERT( DAL_allocArray( &b, 0 ), "fails allocating 0 bytes!" );
		}

		return DAL_dataItemsToString( &b, s, size );
	}

	char buf[64];
	int i;

	s[0] = '\0';

	if( ! size ) {
		return s;
	}
	if( ! DAL_dataSize(data) ) {
		strncat( s, "{}", size );
		return s;
	}

	strncat( s, "{", size );


	switch( data->medium ) {
		case NoMedium: {
			break;
		}
		case File: {
			DAL_ERROR( data, "You shouldn't be here O,o" );
		}
		case Array: {
			for( i = 0; i < DAL_dataSize(data)-1; ++ i ) {
				snprintf( buf, sizeof(buf), "%d,", data->array.data[i] );
				strncat( s, buf, size );
			}

			snprintf( buf, sizeof(buf), "%d", data->array.data[i] );
			strncat( s, buf, size );
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}


	strncat( s, "}", size );

	if( strlen(s) == (unsigned) size-1 ) {
		s[size-4] = s[size-3] = s[size-2] = '.';
		s[size-1] = '}';
	}

	return s;
}


/***************************************************************************************************************/
/********************************************* DAL misc functions **********************************************/
/***************************************************************************************************************/

void DAL_initialize( int *argc, char ***argv )
{
	MPI_Init( argc, argv );

	DAL_init( & DAL_buffer );
	DAL_allocBuffer( & DAL_buffer, 1024*10 ); // TODO: find a better size... 10 MB for now
}
void DAL_finalize( )
{
	DAL_destroy( & DAL_buffer );

	MPI_Finalize( );
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
			if( remove(data->file.name) != 0 ) {
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
dal_size_t DAL_dataSize( Data *data )
{
	switch( data->medium ) {
		case File: {
			/*
			dal_size_t len;
			dal_size_t cursor;
			cursor = ftell( data->file.handle );          // getting current position
			fseek( data->file.handle, 0, SEEK_END );      // going to end
			len = ftell( data->file.handle );             // getting current position
			fseek( data->file.handle, cursor, SEEK_SET ); // getting back to where I was
			return len / sizeof(int);
			*/
			return data->file.size;
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
bool DAL_allocData( Data *data, dal_size_t size )
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


// functions to work with any find of block device (ie: Files)
bool DAL_isDevice( Data *data )
{
	return data->medium == File;
}



dal_size_t DAL_dataCopy( Data *src, Data *dst )
{
	DAL_ASSERT( DAL_dataSize(src) == DAL_dataSize(dst), src, "src and dst size should be the same" );
	return DAL_dataCopyO( src, 0, dst, 0 );
}
dal_size_t DAL_dataCopyO( Data *src, dal_size_t srcOffset, Data *dst, dal_size_t dstOffset )
{
	dal_size_t size = MIN( DAL_dataSize(src)-srcOffset, DAL_dataSize(dst)-dstOffset );
	return DAL_dataCopyOS( src, srcOffset, dst, dstOffset, size );
}
dal_size_t DAL_dataCopyOS( Data *src, dal_size_t srcOffset, Data *dst, dal_size_t dstOffset, dal_size_t size )
{
	DAL_ASSERT( DAL_dataSize(src)-srcOffset >= size, src, "not enough data to copy" );
	DAL_ASSERT( DAL_dataSize(dst)-dstOffset >= size, dst, "not enough space to copy data into" );

	if( src->medium == File ) {
		if( dst->medium == File ) {
			// file -> file
			SPD_DEBUG( "Copying data from file to file" );

			FILE *srcHandle = fopen( src->file.name, "rb" );
			FILE *dstHandle = fopen( dst->file.name, "wb" );

			// allocating a buffer
			Data buffer;
			DAL_init( &buffer );
			SPD_ASSERT( DAL_allocBuffer( &buffer, size ), "memory completely over..." );

			// moving cursors to offsets
			fseek( srcHandle, srcOffset*sizeof(int), SEEK_SET );
			DAL_ASSERT( ftell( srcHandle ) == srcOffset*(dal_size_t)sizeof(int), src, "Error on fseek(%lld)!", srcOffset*sizeof(int) );
			fseek( dstHandle, dstOffset*sizeof(int), SEEK_SET );
			DAL_ASSERT( ftell( dstHandle ) == dstOffset*(dal_size_t)sizeof(int), dst, "Error on fseek(%lld)!", dstOffset*sizeof(int) );

			dal_size_t r1, r2;
			dal_size_t current = 0;

			while( current != size ) {
				r1 = fread( buffer.array.data, sizeof(int), MIN( size-current, DAL_dataSize(&buffer) ), srcHandle );
				SPD_ASSERT( r1 != 0, "Error on fread()!" );
				r2 = fwrite( buffer.array.data, sizeof(int), r1, dstHandle );
				SPD_ASSERT( r1 == r2, "fwrite() couldn't write as much as fread() got..." );

				current += r1;
			}

			fclose( srcHandle );
			fclose( dstHandle );

			return current;
		}
		else if( dst->medium == Array ) {
			// file -> array
			dal_size_t r;
			dal_size_t current = 0;

			FILE *srcHandle = fopen( src->file.name, "rb" );

			fseek( srcHandle, srcOffset*sizeof(int), SEEK_SET );
			DAL_ASSERT( ftell( srcHandle ) == srcOffset*(dal_size_t)sizeof(int), src, "Error on fseek(%lld)!", srcOffset*sizeof(int) );

			while( current != size ) {
				r = fread( dst->array.data + dstOffset + current, sizeof(int), size-current, srcHandle );
				SPD_ASSERT( r != 0, "Error on fread()!" );
				current += r;
			}

			fclose( srcHandle );

			return current;
		}
		else {
			DAL_UNIMPLEMENTED( dst );
		}
	}
	else if( src->medium == Array ) {
		if( dst->medium == File ) {
			// array -> file
			dal_size_t r;
			dal_size_t current = 0;

			FILE *dstHandle = fopen( dst->file.name, "wb" );

			fseek( dstHandle, dstOffset*sizeof(int), SEEK_SET );
			DAL_ASSERT( ftell( dstHandle ) == dstOffset*(dal_size_t)sizeof(int), dst, "Error on fseek(%lld)!", dstOffset*sizeof(int) );

			while( current != size ) {
				r = fwrite( src->array.data + srcOffset, sizeof(int), size, dstHandle );
				SPD_ASSERT( r != 0, "Error on fwrite()!" );
				current += r;
			}

			fclose( dstHandle );

			return current;
		}
		else if( dst->medium == Array ) {
			// array -> array
			SPD_DEBUG( "Copying data from array to array" );

			memcpy( dst->array.data + dstOffset, src->array.data + srcOffset, size*sizeof(int) );
			return size;
		}
		else {
			DAL_UNIMPLEMENTED( dst );
		}
	}
	else {
		DAL_UNIMPLEMENTED( src );
	}

	SPD_ERROR( "How did we get here!?" );
	return -1;
}


// allocating an Array in memory
bool DAL_allocArray( Data *data, dal_size_t size )
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
bool DAL_reallocArray ( Data *data, dal_size_t size )
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
	DAL_ASSERT( DAL_isInitialized(data), data, "data should have been initialized" );

	if( data->medium == Array ) {
		SPD_WARNING( "trying to reallocate an Array as an Array..." );
		return 1;
	}

	Data arrayData;
	DAL_init( &arrayData );
	if( ! DAL_allocArray( &arrayData, data->array.size ) ) {
		return 0;
	}

	DAL_dataCopy( data, &arrayData );

	DAL_destroy( data );
	*data = arrayData;

	return 1;
}

// allocating a temporary buffer in memory (an Array)
bool DAL_allocBuffer( Data *data, dal_size_t size )
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
bool DAL_allocFile( Data *data, dal_size_t size )
{
	DAL_ASSERT( DAL_isInitialized(data), data, "data should have been initialized" );

	FILE *handle;
	char name[128];
	do {
		#define X ( toFileChar(rand()) )
		#define X4 X,X,X,X
		snprintf( name, sizeof(name), "tmp%d_%c%c%c%c-%c%c%c%c-%c%c%c%c-%c%c%c%c", GET_ID(), X4, X4, X4, X4 );
		#undef X4
		#undef X
		handle = fopen( name, "rb" );
	} while( handle ); // if file exists, try again!

	handle = fopen( name, "wb+" );
	if( ! handle ) {
		SPD_DEBUG( "File \"%s\" couldn't be opened: %s", name, strerror(errno) );
	}

	data->medium = File;
	strncpy( data->file.name, name, sizeof(data->file.name) );
	data->file.handle = handle;
	data->file.size = size;

	return 1;
}




/***************************************************************************************************************/
/************************************* [Data] Communication Primitives *****************************************/
/***************************************************************************************************************/
// tiny wrappers for MPI
// these don't use Datas, memory arrays like MPI do, but are so much more comfortable for our purpose...
static inline void DAL_MPI_SEND( void *array, dal_size_t size, MPI_Datatype dataType, int dest ) {
	MPI_Send( array, size, dataType, dest, 0, MPI_COMM_WORLD );
}
static inline dal_size_t DAL_MPI_RECEIVE( void *array, dal_size_t size, MPI_Datatype dataType, int source ) {
	int sizeR;
	MPI_Status stat;
	MPI_Recv( array, size, dataType, source, 0, MPI_COMM_WORLD, &stat );
	MPI_Get_count( &stat, dataType, &sizeR );
	return sizeR;
}
static inline int DAL_MPI_INCOMING_DATA( MPI_Datatype dataType, int source ) {
	int size;
	MPI_Status status;
	MPI_Probe( source, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
	MPI_Get_count( &status, dataType, &size );
	return size;
}






















/**
* @brief Sends data to dest
*
* @param[in] data  		Data to be sent
* @param[in] dest     	Rank of the receiver process
*/
void DAL_send( Data *data, int dest )
{
	DAL_ASSERT( ! DAL_isInitialized(data), data, "data shouldn't have been initialized" );

	/*
	char buf[64];
	DAL_DEBUG( data, "sending %lld items to %d: %s", DAL_dataSize(data), dest, DAL_dataItemsToString(data,buf,sizeof(buf)) );
	*/

	Data buffer;
	DAL_acquireGlobalBuffer( &buffer );

	dal_size_t i = 0;
	dal_size_t r;
	while( i < DAL_BLOCK_COUNT(data, &buffer) ) {
		r = DAL_dataCopyO( data, i * DAL_dataSize(&buffer), &buffer, 0 );
		DAL_MPI_SEND( buffer.array.data, r, MPI_INT, dest );
		++ i;
	}

	DAL_releaseGlobalBuffer( &buffer );

	// TODO: optimize for Array

	/*


			Data buffer = DAL_buffer;
			dal_size_t r;
			dal_size_t total = 0;
			while( total != DAL_dataSize(data) ) {
				buffer = DAL_buffer; // to reset size
				r = DAL_readDataBlock( data, DAL_dataSize(&buffer), 0, &buffer, 0 );
				total += r;
				if( r == 0 ) {
					DAL_ERROR( data, "something went wrong, data should be %lld, but I could only read %lld", DAL_dataSize(data), total );
				}
				buffer.array.size = r;
				DAL_send( &buffer, dest );
			}
			break;
		}
		case Array: {
			DAL_MPI_SEND( data->array.data, DAL_dataSize(data), MPI_INT, dest );
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}
	*/
}

/**
* @brief Receives data from source
*
* @param[in] data		Data buffer to store received elements
* @param[in] size  		Max number of elements to be received
* @param[in] source    	Rank of the sender process
*/
void DAL_receive( Data *data, dal_size_t size, int source )
{
	char buf[64];
	// DAL_DEBUG( data, "receiving %lld items from %d", size, source );

	DAL_ASSERT( DAL_isInitialized(data), data, "data should have been initialized" );

	DAL_allocData( data, size );
	Data buffer;
	DAL_acquireGlobalBuffer( &buffer );

	dal_size_t i = 0;
	dal_size_t r;
	while( i < DAL_BLOCK_COUNT(data, &buffer) ) {
		r = DAL_MPI_RECEIVE( buffer.array.data, DAL_dataSize(&buffer), MPI_INT, source );
		DAL_dataCopyO( &buffer, 0, data, i * DAL_dataSize(&buffer) );
		++ i;
	}

	// DAL_DEBUG( data, "received %lld items from %d: %s", DAL_dataSize(data), source, DAL_dataItemsToString(data,buf,sizeof(buf)) );

	DAL_releaseGlobalBuffer( &buffer );



#if 0
	/*
	char buf[64];
	DAL_DEBUG( data, "receiving %lld items from %d", size, source );
	*/

	if( DAL_allocArray( data, size ) ) {
		// data->medium == Array
		DAL_MPI_RECEIVE( data->array.data, size, MPI_INT, source );
	}
	else if( DAL_allocFile( data, size ) ) {
		Data buffer = DAL_buffer;
		dal_size_t missing = size;
		while( missing != 0 ) {
			if( missing < DAL_dataSize(&buffer) ) {
				buffer.array.size = missing;
			}

			DAL_MPI_RECEIVE( buffer.array.data, DAL_dataSize(&buffer), MPI_INT, source );
			DAL_writeDataBlock( data, DAL_dataSize(&buffer), 0, &buffer, 0 );
			missing -= DAL_dataSize( &buffer );
		}
	}
	else {
		SPD_ERROR( "Couldn't allocate any medium to read %lld items into...", size );
	}

	/*
	DAL_DEBUG( data, "received %lld items from %d: %s", DAL_dataSize(data), source, DAL_dataItemsToString(data,buf,sizeof(buf)) );
	*/
#endif
}

void DAL_sendU( Data *data, int dest )
{
	dal_size_t size = DAL_dataSize(data);
	//SPD_DEBUG( "telling I'll be sending %lld items", size );
	DAL_MPI_SEND( & size, 1, MPI_LONG, dest ); // sending size
	DAL_send( data, dest ); // sending data
}
void DAL_receiveU( Data *data, int source )
{
	dal_size_t size;
	DAL_MPI_RECEIVE( &size, 1, MPI_LONG, source ); // receiving size
	//SPD_DEBUG( "heard I'll be receiving %lld items", size );
	DAL_receive( data, size, source ); // receiving data
}
void DAL_receiveA( Data *data, dal_size_t size, int source )
{
	dal_size_t oldDataSize = DAL_dataSize(data);
		//SPD_DEBUG( "appending %lld items to the %lld I've already got: %s", size, oldDataSize, DAL_dataItemsToString(data,buf,sizeof(buf)) );
	SPD_ASSERT( DAL_reallocArray( data, DAL_dataSize(data) + size ), "not enough memory to allocate data" );
	DAL_MPI_RECEIVE( data->array.data + oldDataSize, size, MPI_INT, source );
		//SPD_DEBUG( "post-appending items: %s", DAL_dataItemsToString(data,buf,sizeof(buf)) );
}
void DAL_receiveAU( Data *data, int source )
{
	dal_size_t size;
	DAL_MPI_RECEIVE( &size, 1, MPI_LONG, source ); // receiving size
	//SPD_DEBUG( "heard I'll be appending %lld items to the %lld I've already got", size, data->array.size );
	DAL_receiveA( data, size, source ); // receiving data
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
dal_size_t DAL_sendrecv( Data *sdata, dal_size_t scount, dal_size_t sdispl, Data* rdata, dal_size_t rcount, dal_size_t rdispl, int partner )
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
			DAL_ASSERT( data->array.size % GET_N() == 0, data, "data should be a multiple of %d (but it's %lld)", GET_N(), data->array.size );
			MPI_Scatter( data->array.data, data->array.size/GET_N(), MPI_INT, MPI_IN_PLACE, data->array.size/GET_N(), MPI_INT, GET_ID(), MPI_COMM_WORLD );
			SPD_ASSERT( DAL_reallocArray( data, data->array.size/GET_N() ), "not enough memory to allocate data" );
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}
}
void DAL_scatterReceive( Data *data, dal_size_t size, int root )
{
	SPD_ASSERT( DAL_allocArray( data, size ), "not enough memory to allocate data" );
	MPI_Scatter( NULL, 0, MPI_INT, data->array.data, size, MPI_INT, root, MPI_COMM_WORLD );
}
/**
* @brief Scatters data among all processes
*
* @param[in] data  		Data to be scattered
* @param[in] size     	Number of elements per process
* @param[in] root     	Rank of the root process
*/
void DAL_scatter( Data *data, dal_size_t size, int root )
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
void DAL_gatherReceive( Data *data, dal_size_t size )
{
	SPD_ASSERT( size % GET_N() == 0, "size should be a multiple of %d (but it's %lld)", GET_N(), size );
	SPD_ASSERT( DAL_reallocArray( data, size ), "not enough memory to allocate data" );
	MPI_Gather( MPI_IN_PLACE, size/GET_N(), MPI_INT, data->array.data, size/GET_N(), MPI_INT, GET_ID(), MPI_COMM_WORLD );
}
/**
* @brief Gathers data from all processes
*
* @param[in,out] 	data  		Data to be gathered/sent
* @param[in] 		size     	Number of elements to be gathered
* @param[in] 		root     	Rank of the root process
*/
void DAL_gather( Data *data, dal_size_t size, int root )
{
	if( GET_ID() == root ) {
		return DAL_gatherReceive( data, size );
	}
	else {
		return DAL_gatherSend( data, root );
	}
}







void DAL_scattervSend( Data *data, dal_size_t *counts, dal_size_t *displs )
{
	int sc[GET_N()];
	int sd[GET_N()];
	int i, j;

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	DAL_ASSERT( globalBuf.array.size >= GET_N(), &globalBuf, "The global-buffer is too small for a scatter communication (its size is %lld, but there are %d processes)", globalBuf.array.size, GET_N() );

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	dal_size_t max_count = 0;
	for ( i=0; i<GET_N(); i++ )
		if ( counts[i] > max_count )
			max_count = counts[i];
	MPI_Bcast( &max_count, 1, MPI_LONG, GET_ID(), MPI_COMM_WORLD );
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
void DAL_scattervReceive( Data *data, dal_size_t size, int root )
{
	int i, j;

	SPD_ASSERT( DAL_isInitialized(data), "Data should be initialized" );
	SPD_ASSERT( DAL_allocData( data, size ), "not enough space to allocate data" );

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	dal_size_t max_count;
	MPI_Bcast( &max_count, 1, MPI_LONG, GET_ID(), MPI_COMM_WORLD );
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
void DAL_scatterv( Data *data, dal_size_t *counts, dal_size_t *displs, int root )
{
	if( GET_ID() == root ) {
		return DAL_scattervSend( data, counts, displs );
	}
	else {
		return DAL_scattervReceive( data, counts[GET_ID()], root );
	}
}





void DAL_gathervSend( Data *data, int root )
{
	int i, j;

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	dal_size_t max_count;
	dal_size_t size = DAL_dataSize(data);
	MPI_Allreduce( &size, &max_count, 1, MPI_LONG, MPI_MAX, MPI_COMM_WORLD );
	int num_iterations = max_count / blockSize + (max_count % blockSize > 0);
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
void DAL_gathervReceive( Data *data, dal_size_t *counts, dal_size_t *displs )
{
	int rc[GET_N()];
	int rd[GET_N()];
	int i, j;

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	DAL_ASSERT( globalBuf.array.size >= GET_N(), &globalBuf, "The global-buffer is too small for a gather communication (its size is %lld, but there are %d processes)", globalBuf.array.size, GET_N() );

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	dal_size_t max_count = 0;
	MPI_Allreduce( &counts[GET_ID()], &max_count, 1, MPI_LONG, MPI_MAX, MPI_COMM_WORLD );
	int num_iterations = max_count / blockSize + (max_count % blockSize > 0);
	int r, tmp;

	//Computing the total number of elements to be received
	dal_size_t rcount = 0;
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
void DAL_gatherv( Data *data, dal_size_t *counts, dal_size_t *displs, int root )
{
	if( GET_ID() == root ) {
		return DAL_gathervReceive( data, counts, displs );
	}
	else {
		return DAL_gathervSend( data, root );
	}
}







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
	DAL_ASSERT( buf->array.size % parts == 0, buf, "Data size should be a multiple of %d, but it's %lld", parts, buf->array.size );
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
void DAL_alltoall( Data *data, dal_size_t count )
{
	if( data->medium != File && data->medium != Array )
		DAL_UNSUPPORTED( data );

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	DAL_ASSERT( globalBuf.array.size >= GET_N()*2, &globalBuf, "The global-buffer is too small for an alltoall communication (its size is %lld, but there are %d processes)", globalBuf.array.size, GET_N() );

	Data bufs[2];
	DAL_splitBuffer( &globalBuf, 2, bufs );
	Data *sendBuf = &bufs[0];
	Data *recvBuf = &bufs[1];

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










/**
* @brief Sends data from all to all processes
*
* @param[in,out] 	data  		Data to be sent and in which receive
* @param[in] 		scounts  	Array containing the number of elements to be sent to each process
* @param[in] 		sdispls     Array of displacements
* @param[in] 		rcounts  	Array containing the number of elements to be received from each process
* @param[in] 		rdispls     Array of displacements
*/
void DAL_alltoallv( Data *data, dal_size_t *scounts, dal_size_t *sdispls, dal_size_t *rcounts, dal_size_t *rdispls )
{
	if( data->medium != File && data->medium != Array )
		DAL_UNSUPPORTED( data );

	int i, j;
	int sc[GET_N()];
	int sd[GET_N()];
	int rc[GET_N()];
	int rd[GET_N()];
	dal_size_t rcount = 0;

	//Computing the total number of elements to be received
	for ( i=0; i<GET_N(); i++ )
		rcount += rcounts[i];

	//Data to store the received elements
	Data recvData;
	DAL_init( &recvData );
	SPD_ASSERT( DAL_allocData(&recvData, rcount), "not enough space to allocate data" );

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	DAL_ASSERT( globalBuf.array.size >= GET_N()*2, &globalBuf, "The global-buffer is too small for an alltoall communication (its size is %lld, but there are %d processes)", globalBuf.array.size, GET_N() );

	Data bufs[2];
	DAL_splitBuffer( &globalBuf, 2, bufs );
	Data *sendBuf = &bufs[0];
	Data *recvBuf = &bufs[1];

	int blockSize = DAL_dataSize(sendBuf) / GET_N();

	//Retrieving the number of iterations
	dal_size_t max_count;
	MPI_Allreduce( &rcount, &max_count, 1, MPI_LONG, MPI_MAX, MPI_COMM_WORLD );
	int num_iterations = max_count / blockSize + (max_count % blockSize > 0);
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
void DAL_bcastReceive( Data *data, dal_size_t size, int root )
{
	SPD_ASSERT( DAL_allocArray( data, size ), "not enough memory to allocate data" );
	MPI_Bcast( data->array.data, size, MPI_INT, root, MPI_COMM_WORLD );
}
/**
* @brief Broadcasts data to processes
*
* @param[in] data  		Data to be scattered
* @param[in] size     	Number of elements per process
* @param[in] root     	Rank of the root process
*/
void DAL_bcast( Data *data, dal_size_t size, int root )
{
	if( GET_ID() == root ) {
		return DAL_bcastSend( data );
	}
	else {
		return DAL_bcastReceive( data, size, root );
	}
}

/*--------------------------------------------------------------------------------------------------------------*/

