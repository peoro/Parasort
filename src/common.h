#ifndef _COMMON_H_
#define _COMMON_H_

#include "sorting.h"
#include "dal.h"

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
extern "C" {
#endif

//efficiently computes the logarithm base 2 of a positive integer
int _log2 ( unsigned int n );
int _pow2( int n );

//computes the logarithm base k of a positive integer
int _logk ( unsigned int n, unsigned int k );


//compare two integers
int compare (const void * a, const void * b);

//returns 1 if x is a power of two, 0 otherwise
int isPowerOfTwo( int x );

/// sequential sort function
void sequentialSort( const TestInfo *ti, Data *data );

int getBucketIndex( const int *ele, const int *splitters, const long length );
void chooseSplitters( int *array, const long length, const int n, int *newSplitters );
void chooseSplittersFromData( Data *data, const int n, int *newSplitters );

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
}
#endif

#endif
