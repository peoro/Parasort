
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <mpi.h>
#include "dal.h"
#include "dal_internals.h"

#ifndef SPD_PIANOSA
	#define MPI_DST MPI_LONG
#else
	#define MPI_DST MPI_LONG_LONG
#endif

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
	DAL_ASSERT( buf->array.size % parts == 0, buf, "Data size should be a multiple of %d, but it's "DST"", parts, buf->array.size );
	int i;
	for ( i=0; i<parts; i++ ) {
		bufs[i] = *buf;
		bufs[i].array.data = buf->array.data+(buf->array.size/parts)*i;
		bufs[i].array.size /= parts;
	}
}

dal_size_t DAL_allowedBufSize( )
{
	//TODO: find the optimal value
	return 128*1024*1024;
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

static dal_size_t GET_FILE_SIZE( const char *path )
{
	struct stat sb;

	SPD_ASSERT( stat(path, &sb) != -1, "Unable to load statistics for file %s", path );

	return sb.st_size;
}

dal_size_t DAL_getFileCursor( Data *d )
{
	DAL_ASSERT( d->medium == File, d, "Data should be of type File" );

#ifndef SPD_PIANOSA
	return ftello( d->file.handle ) / sizeof(int);
#else
	dal_size_t returnValue;
	SPD_ASSERT( fgetpos64( d->file.handle, &returnValue ) == 0, "fgetpos64 error: %s", strerror(errno) );
	return returnValue / sizeof(int);
#endif
}
void DAL_setFileCursor( Data *d, dal_size_t offset )
{
	DAL_ASSERT( d->medium == File, d, "Data should be of type File" );

#ifndef SPD_PIANOSA
	fseeko( d->file.handle, offset*sizeof(int), SEEK_SET );
#else
	fseeko64( d->file.handle, offset*sizeof(int), SEEK_SET );
#endif

	DAL_ASSERT( DAL_getFileCursor( d ) == offset, d, "DAL_setFileCursor error: file cursor = "DST" while offset = "DST, DAL_getFileCursor( d ), offset );
}
static void READ_FILE( FILE *f, int *buf, dal_size_t size )
{
	dal_size_t r;
	dal_size_t current = 0;

	while( current != size ) {
		r = fread( buf, sizeof(int), size-current, f );
		SPD_ASSERT( r != 0, "Error on fread(): %s", strerror(errno)  );
		current += r;
	}
}
static void WRITE_FILE( FILE *f, int *buf, dal_size_t size )
{
	dal_size_t w;
	dal_size_t current = 0;

	while( current != size ) {
		w = fwrite( buf, sizeof(int), size-current, f );
		SPD_ASSERT( w != 0, "Error on fwrite(): %s", strerror(errno) );
		current += w;
	}
}

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
			snprintf( s, size, "\"%s\" [file on disk] of "DST" items", d->file.name, DAL_dataSize(d) );
			break;
		case Array:
			snprintf( s, size, "array [on principal memory @ %p] of "DST" items", d->array.data, DAL_dataSize(d) );
			break;
		default:
			snprintf( s, size, "Unknow medium code %d [%s]", d->medium, DAL_mediumName(d->medium) );
	}
	return s;
}
char *peoros_strncat( const char *src, size_t srcSize, char *dst, size_t dstSize )
{
	size_t n = MIN( srcSize, dstSize - strlen(dst) - 1 );
	return strncat( dst, src, n );
}
char *peoros_strncat2( const char *src, char *dst, size_t dstSize )
{
	return peoros_strncat( src, strlen(src), dst, dstSize );
}
char * DAL_dataItemsToString( Data *data, char *s, int size )
{
	if( data->medium == File ) {
		Data b;

		DAL_init( &b );

		if( DAL_dataSize(data) ) {
			SPD_ASSERT( DAL_allocArray( &b, DAL_dataSize(data) ), "memory completely over..." );
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
	s[ size-1 ] = '\0';

	if( ! size ) {
		return s;
	}
	if( ! DAL_dataSize(data) ) {
		peoros_strncat2( "{}", s, size );
		return s;
	}

	peoros_strncat2( "{", s, size );


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
				peoros_strncat2( buf, s, size );
			}

			snprintf( buf, sizeof(buf), "%d", data->array.data[i] );
			peoros_strncat2( buf, s, size );
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}


	peoros_strncat2( "}", s, size );

	if( strlen(s) == (unsigned) size-1 ) {
		s[size-5] = s[size-4] = s[size-3] = '.';
		s[size-2] = '}';
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
	DAL_allocBuffer( & DAL_buffer, 32*1024*1024 ); // TODO: find a better size... 128MB (32 Mega integers) for now
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
			//GET_FILE_SIZE( data->file.name );
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

	if( DAL_allocArray(data, size) ) {
		return 1;
	}

	if( DAL_allocFile(data, size) ) {
		return 1;
	}

	return 0;
}
bool DAL_reallocData( Data *data, dal_size_t size )
{
	// DAL_DEBUG( data, "reallocing to "DST, size );
	switch ( data->medium ) {
		case File: {
			//TODO: resize the file if size < data->file.size
			data->file.size = size;
			return 1;
		}

		case Array: {
			if( DAL_reallocArray(data, size) ) {
				return 1;
			}
			Data tmp;
			DAL_init( &tmp );

			if( DAL_allocFile(&tmp, size) ) {
				DAL_destroy( data );
				*data = tmp;
				return 1;
			}
			break;
		}

		default: {
			DAL_UNSUPPORTED( data );
			break;
		}
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

	/*
	char buf1[128], buf2[128];
	SPD_DEBUG( "DAL_dataCopyOS( %s, "DST", %s, "DST", "DST" );",
	           DAL_dataToString( src, buf1, sizeof(buf1) ), srcOffset,
	           DAL_dataToString( dst, buf2, sizeof(buf2) ), dstOffset,
	           size );
	*/

	if( src->medium == File ) {
		if( dst->medium == File ) {
			// file -> file
 			//SPD_DEBUG( "Copying data from file to file" );

			// allocating a buffer
			Data buffer;
			DAL_init( &buffer );
			SPD_ASSERT( DAL_allocBuffer( &buffer, 1024*1024 ), "memory completely over..." );

			// moving cursors to offsets
 			DAL_setFileCursor( src, srcOffset );
			DAL_setFileCursor( dst, dstOffset );

			dal_size_t toReadSize;
			dal_size_t current = 0;

			while( current != size ) {
				toReadSize = MIN( size-current, DAL_dataSize(&buffer) );
				READ_FILE( src->file.handle, buffer.array.data, toReadSize );
				WRITE_FILE( dst->file.handle, buffer.array.data, toReadSize );

				current += toReadSize;
			}

			DAL_destroy( &buffer );

			return current;
		}
		else if( dst->medium == Array ) {
			// file -> array
 			//SPD_DEBUG( "Copying data from file to array" );

			DAL_setFileCursor( src, srcOffset );
			READ_FILE( src->file.handle, dst->array.data+dstOffset, size );
			return size;
		}
		else {
			DAL_UNSUPPORTED( dst );
		}
	}
	else if( src->medium == Array ) {
		if( dst->medium == File ) {
			// array -> file
// 			SPD_DEBUG( "Copying data from array to file" );

			DAL_setFileCursor( dst, dstOffset );
			WRITE_FILE( dst->file.handle, src->array.data+srcOffset, size );
			return size;
		}
		else if( dst->medium == Array ) {
			// array -> array
// 			SPD_DEBUG( "Copying data from array to array" );
			memcpy( dst->array.data + dstOffset, src->array.data + srcOffset, size*sizeof(int) );
			return size;
		}
		else {
			DAL_UNSUPPORTED( dst );
		}
	}
	else {
		DAL_UNSUPPORTED( src );
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

	// TODO: threshold must be parametric
	const dal_size_t threshold = DAL_allowedBufSize();

	// TODO: maybe add a threshold on max size
	if( size > threshold ) { return 0; }

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
	// TODO: threshold must be parametric
	const dal_size_t threshold = DAL_allowedBufSize();

	// TODO: maybe add a threshold on max size
	if( size > threshold ) { return 0; }

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
	if( ! DAL_allocArray( &arrayData, data->file.size ) ) {
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
	const dal_size_t threshold = DAL_allowedBufSize();
	if ( size > threshold )
		size = threshold;

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


void DAL_dataSwap( Data *a, Data *b )
{
	Data tmp = *a;
	*a = *b;
	*b = tmp;
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

// 	char buf[64];
// 	DAL_DEBUG( data, "sending "DST" items to %d: %s", DAL_dataSize(data), dest, DAL_dataItemsToString(data,buf,sizeof(buf)) );

	dal_size_t i;
	dal_size_t r;
	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	switch( data->medium ) {
		case File: {
			for ( i=0; i<DAL_BLOCK_COUNT(data, &globalBuf); i++ ) {
				r = DAL_dataCopyO( data, i * DAL_dataSize(&globalBuf), &globalBuf, 0 );
				DAL_MPI_SEND( globalBuf.array.data, r, MPI_INT, dest );
			}
			break;
		}
		case Array: {
			dal_size_t offset;
			for ( i=0; i<DAL_BLOCK_COUNT(data, &globalBuf); i++ ) {
				offset = i*DAL_dataSize(&globalBuf);
				r = MIN( DAL_dataSize(data)-offset, DAL_dataSize(&globalBuf) );
				DAL_MPI_SEND( data->array.data+offset, r, MPI_INT, dest );
			}
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}
	DAL_releaseGlobalBuffer( &globalBuf );
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
// 	char buf[64];
// 	DAL_DEBUG( data, "receiving "DST" items from %d", size, source );

	SPD_ASSERT( DAL_allocData( data, size ), "not enough space to allocate data" );

	dal_size_t i;
	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	switch( data->medium ) {
		case File: {
			dal_size_t r;
			for ( i=0; i<DAL_BLOCK_COUNT(data, &globalBuf); i++ ) {
				r = DAL_MPI_RECEIVE( globalBuf.array.data, DAL_dataSize(&globalBuf), MPI_INT, source );
				DAL_dataCopyO( &globalBuf, 0, data, i * DAL_dataSize(&globalBuf) );
			}
			break;
		}
		case Array: {
			for ( i=0; i<DAL_BLOCK_COUNT(data, &globalBuf); i++ ) {
				DAL_MPI_RECEIVE( data->array.data+i*DAL_dataSize(&globalBuf), DAL_dataSize(&globalBuf), MPI_INT, source );
			}
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}
	DAL_releaseGlobalBuffer( &globalBuf );
}

void DAL_sendU( Data *data, int dest )
{
	dal_size_t size = DAL_dataSize(data);
	//SPD_DEBUG( "telling I'll be sending "DST" items", size );
	DAL_MPI_SEND( & size, 1, MPI_DST, dest ); // sending size
	DAL_send( data, dest ); // sending data
}
void DAL_receiveU( Data *data, int source )
{
	dal_size_t size;
	DAL_MPI_RECEIVE( &size, 1, MPI_DST, source ); // receiving size
	//SPD_DEBUG( "heard I'll be receiving "DST" items", size );
	DAL_receive( data, size, source ); // receiving data
}
void DAL_receiveA( Data *data, dal_size_t size, int source )
{
	dal_size_t oldDataSize = DAL_dataSize(data);
		//SPD_DEBUG( "appending "DST" items to the "DST" I've already got: %s", size, oldDataSize, DAL_dataItemsToString(data,buf,sizeof(buf)) );
	SPD_ASSERT( DAL_reallocData( data, DAL_dataSize(data) + size ), "not enough memory to allocate data" );

	//DAL_MPI_RECEIVE( data->array.data + oldDataSize, size, MPI_INT, source );
	Data tmp;
	DAL_init( &tmp );
	DAL_receive( &tmp, size, source );

	DAL_dataCopyOS( &tmp, 0, data, oldDataSize, size );

	DAL_destroy( &tmp );
		//SPD_DEBUG( "post-appending items: %s", DAL_dataItemsToString(data,buf,sizeof(buf)) );
}
void DAL_receiveAU( Data *data, int source )
{
	dal_size_t size;
	DAL_MPI_RECEIVE( &size, 1, MPI_DST, source ); // receiving size
	//SPD_DEBUG( "heard I'll be appending "DST" items to the "DST" I've already got", size, data->array.size );
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
	int i, sc, rc, tmp;	
	dal_size_t oldSize = 0;
	dal_size_t recvCount = 0;
	MPI_Status 	status;

	if( rdata->medium == NoMedium ) {
		SPD_ASSERT( DAL_allocData( rdata, rcount ), "not enough space to allocate data" );
	}
	else {
		oldSize = DAL_dataSize( rdata );
		SPD_ASSERT( DAL_reallocData( rdata, oldSize+rcount ), "not enough space to allocate data" );
	}
	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	//Retrieving the number of iterations	
	dal_size_t max_count = MAX( scount, rcount );
	int blockSize = DAL_dataSize(&globalBuf) / 2;
	int num_iterations = max_count / blockSize + (max_count % blockSize > 0);
	
	if ( sdata->medium == File || rdata->medium == File ) {
		Data bufs[2];
		DAL_splitBuffer( &globalBuf, 2, bufs );
		Data *sendBuf = &bufs[0];
		Data *recvBuf = &bufs[1];
		
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
			
			if ( sc < blockSize && rc < blockSize )
				break;
		}
	}
	else if ( sdata->medium == Array && rdata->medium == Array ) {
		
		for ( i=0; i<num_iterations; i++ ) {
			tmp = MIN( blockSize, (scount-i*blockSize) );		//Number of elements to be sent to the partner process by MPI_Sendrecv
			sc = tmp > 0 ? tmp : 0;
			tmp = MIN( blockSize, (rcount-i*blockSize) );		//Number of elements to be received from the partner process by MPI_Sendrecv
			rc = tmp > 0 ? tmp : 0;

			MPI_Sendrecv(sdata->array.data+sdispl+i*blockSize, sc, MPI_INT, partner, 100, rdata->array.data+rdispl+recvCount, rc, MPI_INT, partner, 100, MPI_COMM_WORLD, &status );
			MPI_Get_count( &status, MPI_INT, &rc );
			recvCount += rc;			
			
			if ( sc < blockSize && rc < blockSize )
				break;
		}						
	}
	else
		DAL_UNSUPPORTED( sdata );
	SPD_ASSERT( DAL_reallocData( rdata, oldSize+recvCount ), "not enough space to allocate data" );
	DAL_releaseGlobalBuffer( &globalBuf );

	return recvCount;
}






void DAL_scatterSend( Data *data )
{
	int i, j;

	SPD_ASSERT( DAL_dataSize(data) % GET_N() == 0, "Data size should be a multiple of n=%d, but it's "DST, GET_N(), DAL_dataSize(data) );

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	DAL_ASSERT( globalBuf.array.size >= GET_N(), &globalBuf, "The global-buffer is too small for a scatter communication (its size is "DST", but there are %d processes)", globalBuf.array.size, GET_N() );

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	dal_size_t count = DAL_dataSize(data) / GET_N();
	int num_iterations = count / blockSize + (count % blockSize > 0);
	int tmp;
	int sc[GET_N()], sd[GET_N()];

	switch( data->medium ) {
		case File: {

			for ( i=0; i<num_iterations; i++ ) {
				tmp = MIN( blockSize, (count-i*blockSize) );

				for ( j=0; j<GET_N(); j++ ) {
					sc[j] = tmp;
					sd[j] = j*tmp;
					DAL_dataCopyOS( data, j*count + i*blockSize, &globalBuf, sd[j], sc[j] );
				}

				MPI_Scatterv( globalBuf.array.data, sc, sd, MPI_INT, MPI_IN_PLACE, sc[GET_ID()], MPI_INT, GET_ID(), MPI_COMM_WORLD );
			}
			break;
		}
		case Array: {

			for ( i=0; i<num_iterations; i++ ) {
				tmp = MIN( blockSize, (count-i*blockSize) );

				for ( j=0; j<GET_N(); j++ ) {
					sc[j] =	tmp;	//Number of elements to be sent to process j by MPI_Alltoallv
					sd[j] = j*count + i*blockSize;
				}
				MPI_Scatterv( data->array.data, sc, sd, MPI_INT, MPI_IN_PLACE, sc[GET_ID()], MPI_INT, GET_ID(), MPI_COMM_WORLD );
			}
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}

	SPD_ASSERT( DAL_reallocData( data, count ), "not enough space to reallocate data" );
	DAL_releaseGlobalBuffer( &globalBuf );
}
void DAL_scatterReceive( Data *data, dal_size_t count, int root )
{
	int i, j;

	SPD_ASSERT( DAL_allocData( data, count ), "not enough space to allocate data" );

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	int num_iterations = count / blockSize + (count % blockSize > 0);
	int tmp;

	switch( data->medium ) {
		case File: {

			for ( i=0; i<num_iterations; i++ ) {
				tmp = MIN( blockSize, count-i*blockSize );
				MPI_Scatterv( NULL, NULL, NULL, MPI_INT, globalBuf.array.data, tmp, MPI_INT, root, MPI_COMM_WORLD );

				DAL_dataCopyOS( &globalBuf, 0, data, i*blockSize, tmp );
			}
			break;
		}
		case Array: {
			int recvDispl = 0;

			for ( i=0; i<num_iterations; i++ ) {
				tmp = MIN( blockSize, count-i*blockSize );
				MPI_Scatterv( NULL, NULL, NULL, MPI_INT, data->array.data+recvDispl, tmp, MPI_INT, root, MPI_COMM_WORLD );
				recvDispl += tmp;
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
* @param[in] data  		Data to be scattered
* @param[in] count     	Number of elements per process
* @param[in] root     	Rank of the root process
*/
void DAL_scatter( Data *data, dal_size_t count, int root )
{
	if( GET_ID() == root ) {
		return DAL_scatterSend( data );
	}
	else {
		return DAL_scatterReceive( data, count, root );
	}
}





void DAL_gatherSend( Data *data, int root )
{
	int i, j;

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

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
void DAL_gatherReceive( Data *data, dal_size_t size )
{
	SPD_ASSERT( size % GET_N() == 0, "size should be a multiple of %d (but it's "DST")", GET_N(), size );
	int rc[GET_N()];
	int rd[GET_N()];
	int i, j;

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	DAL_ASSERT( globalBuf.array.size >= GET_N(), &globalBuf, "The global-buffer is too small for a gather communication (its size is "DST", but there are %d processes)", globalBuf.array.size, GET_N() );

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
void DAL_gather( Data *data, dal_size_t count, int root )
{
	if( GET_ID() == root ) {
		return DAL_gatherReceive( data, count*GET_N() );
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

	DAL_ASSERT( globalBuf.array.size >= GET_N(), &globalBuf, "The global-buffer is too small for a scatter communication (its size is "DST", but there are %d processes)", globalBuf.array.size, GET_N() );

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	dal_size_t max_count = 0;
	for ( i=0; i<GET_N(); i++ )
		if ( counts[i] > max_count )
			max_count = counts[i];
	MPI_Bcast( &max_count, 1, MPI_DST, GET_ID(), MPI_COMM_WORLD );
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
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}

	SPD_ASSERT( DAL_reallocData( data, counts[GET_ID()] ), "not enough space to reallocate data" );
	DAL_releaseGlobalBuffer( &globalBuf );
}
void DAL_scattervReceive( Data *data, dal_size_t count, int root )
{
	int i, j;

	SPD_ASSERT( DAL_isInitialized(data), "Data should be initialized" );
	SPD_ASSERT( DAL_allocData( data, count ), "not enough space to allocate data" );

	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	dal_size_t max_count;
	MPI_Bcast( &max_count, 1, MPI_DST, root, MPI_COMM_WORLD );
	int num_iterations = max_count / blockSize + (max_count % blockSize > 0);
	int recvCount, tmp;

	switch( data->medium ) {
		case File: {

			for ( i=0; i<num_iterations; i++ ) {
				tmp = MIN( blockSize, count-i*blockSize );
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
				tmp = MIN( blockSize, count-i*blockSize );
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
	MPI_Allreduce( &size, &max_count, 1, MPI_DST, MPI_MAX, MPI_COMM_WORLD );
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

	DAL_ASSERT( globalBuf.array.size >= GET_N(), &globalBuf, "The global-buffer is too small for a gather communication (its size is "DST", but there are %d processes)", globalBuf.array.size, GET_N() );

	int blockSize = DAL_dataSize(&globalBuf) / GET_N();

	//Retrieving the number of iterations
	dal_size_t max_count = 0;
	MPI_Allreduce( &counts[GET_ID()], &max_count, 1, MPI_DST, MPI_MAX, MPI_COMM_WORLD );
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
				rc[j] =	tmp > 0 ? tmp : 0;	//Number of elements to be sent to process j by MPI_toallv
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

	DAL_ASSERT( globalBuf.array.size >= GET_N()*2, &globalBuf, "The global-buffer is too small for an alltoall communication (its size is "DST", but there are %d processes)", globalBuf.array.size, GET_N() );

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

	DAL_ASSERT( globalBuf.array.size >= GET_N()*2, &globalBuf, "The global-buffer is too small for an alltoall communication (its size is "DST", but there are %d processes)", globalBuf.array.size, GET_N() );

	Data bufs[2];
	DAL_splitBuffer( &globalBuf, 2, bufs );
	Data *sendBuf = &bufs[0];
	Data *recvBuf = &bufs[1];

	int blockSize = DAL_dataSize(sendBuf) / GET_N();

	//Retrieving the number of iterations
	dal_size_t max_count;
	MPI_Allreduce( &rcount, &max_count, 1, MPI_DST, MPI_MAX, MPI_COMM_WORLD );
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
	dal_size_t i;
	dal_size_t r;
	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	switch( data->medium ) {
		case File: {
			for ( i=0; i<DAL_BLOCK_COUNT(data, &globalBuf); i++ ) {
				r = DAL_dataCopyO( data, i * DAL_dataSize(&globalBuf), &globalBuf, 0 );
				MPI_Bcast( globalBuf.array.data, r, MPI_INT, GET_ID(), MPI_COMM_WORLD );
			}
			break;
		}
		case Array: {
			dal_size_t offset;
			for ( i=0; i<DAL_BLOCK_COUNT(data, &globalBuf); i++ ) {
				offset = i*DAL_dataSize(&globalBuf);
				r = MIN( DAL_dataSize(data)-offset, DAL_dataSize(&globalBuf) );
				MPI_Bcast( data->array.data+offset, r, MPI_INT, GET_ID(), MPI_COMM_WORLD );
			}
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}
	DAL_releaseGlobalBuffer( &globalBuf );

}
void DAL_bcastReceive( Data *data, dal_size_t size, int root )
{
	SPD_ASSERT( DAL_allocData( data, size ), "not enough space to allocate data" );

	dal_size_t i;
	Data globalBuf;
	DAL_acquireGlobalBuffer( &globalBuf );

	switch( data->medium ) {
		case File: {
			dal_size_t r;
			for ( i=0; i<DAL_BLOCK_COUNT(data, &globalBuf); i++ ) {
				r = MPI_Bcast( globalBuf.array.data, DAL_dataSize(&globalBuf), MPI_INT, root, MPI_COMM_WORLD );
				DAL_dataCopyO( &globalBuf, 0, data, i * DAL_dataSize(&globalBuf) );
			}
			break;
		}
		case Array: {
			for ( i=0; i<DAL_BLOCK_COUNT(data, &globalBuf); i++ ) {
				MPI_Bcast( data->array.data+i*DAL_dataSize(&globalBuf), DAL_dataSize(&globalBuf), MPI_INT, root, MPI_COMM_WORLD );
			}
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}
	DAL_releaseGlobalBuffer( &globalBuf );
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

