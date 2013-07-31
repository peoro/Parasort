
#ifndef _DAL_INTERNALS_H_
#define _DAL_INTERNALS_H_

#include "dal.h"

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
extern "C" {
#endif


// they return how much they copied
dal_size_t DAL_dataCopy( Data *src, Data *dst ); // src.size must be equal to dst.size
dal_size_t DAL_dataCopyO( Data *src, dal_size_t srcOffset, Data *dst, dal_size_t dstOffset ); // read the min of (src+off).size and (dst+off).size
dal_size_t DAL_dataCopyOS( Data *src, dal_size_t srcOffset, Data *dst, dal_size_t dstOffset, dal_size_t size );



// allocating an Array in memory
bool DAL_allocArray( Data *data, dal_size_t size );
bool DAL_reallocArray ( Data *data, dal_size_t size );
bool DAL_reallocAsArray( Data *data ); // tries to realloc data as an array

// allocating a temporary buffer in memory (an Array)
bool DAL_allocBuffer( Data *data, dal_size_t size ); // allocs a buffer of size equal OR LESSER than size

// allocating a File
bool DAL_allocFile( Data *data, dal_size_t size );




void DAL_acquireGlobalBuffer( Data *data );
void DAL_releaseGlobalBuffer( Data *data );



#define DAL_BLOCK_COUNT( data, buf ) ( ( DAL_dataSize(data) / DAL_dataSize(buf) ) + ( DAL_dataSize(data) % DAL_dataSize(buf) > 0 ) )




/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
}
#endif

#endif

