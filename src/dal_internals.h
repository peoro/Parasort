
#ifndef _DAL_INTERNALS_H_
#define _DAL_INTERNALS_H_

#include "dal.h"

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
extern "C" {
#endif


// they return how much they copied
long DAL_dataCopy( Data *src, Data *dst ); // src.size must be equal to dst.size
long DAL_dataCopyO( Data *src, long srcOffset, Data *dst, long dstOffset ); // read the min of (src+off).size and (dst+off).size
long DAL_dataCopyOS( Data *src, long srcOffset, Data *dst, long dstOffset, long size );



// allocating an Array in memory
bool DAL_allocArray( Data *data, long size );
bool DAL_reallocArray ( Data *data, long size );
bool DAL_reallocAsArray( Data *data ); // tries to realloc data as an array

// allocating a temporary buffer in memory (an Array)
bool DAL_allocBuffer( Data *data, long size ); // allocs a buffer of size equal OR LESSER than size

// allocating a File
bool DAL_allocFile( Data *data, long size );




void DAL_acquireGlobalBuffer( Data *data );
void DAL_releaseGlobalBuffer( Data *data );



#define DAL_BLOCK_COUNT( data, buf ) ( ( DAL_dataSize(data) / DAL_dataSize(buf) ) + ( DAL_dataSize(data) % DAL_dataSize(buf) > 0 ) )




/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
}
#endif

#endif

