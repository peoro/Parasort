#ifndef _UTILS_H_
#define _UTILS_H_

#include "sorting.h"
#include "dal.h"

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
extern "C" {
#endif

//efficiently computes the logarithm base 2 of a positive integer
inline int _log2 ( unsigned int n )
{
    unsigned int toRet = 0;
    int m = n - 1;
    while (m > 0) {
		m >>= 1;
		toRet++;
    }
    return toRet;
}
inline int _pow2( int n ) {
	int i, r = 1;
	for( i = 0; i < n; ++ i ) {
		r *= 2;
	}
	return r;
}

//computes the logarithm base k of a positive integer
inline int _logk ( unsigned int n, unsigned int k )
{
    return _log2 ( n ) / _log2 ( k );
}


//compare two integers
inline int compare (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}
/// sequential sort function
void sequentialSort( const TestInfo *ti, Data *data )
{
	switch( data->medium ) {
		case File: {
			DAL_UNIMPLEMENTED( data );
			break;
		}
		case Array: {
			qsort( data->array.data, data->array.size, sizeof(int), compare );
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}
}

//returns 1 if x is a power of two, 0 otherwise
inline int isPowerOfTwo( int x )
{
    return x > 0 && (x & (x - 1)) == 0;
}

/**
* @brief Gets the index of the bucket in which to insert ele
*
* @param[in] ele        The element to be inserted within a bucket
* @param[in] splitters  The array of splitters
* @param[in] length     The number of splitters
*
* @return The index of the bucket in which to insert ele
*/
int getBucketIndex( const int *ele, const int *splitters, const int length )
{
	int low = 0;
	int high = length;
	int cmpResult = 0;
	int mid;

	while ( high > low ) {
		mid = (low + high) >> 1;
		cmpResult = compare( ele, &splitters[mid] );

		if ( cmpResult == 0 )
			return mid;

		if ( cmpResult > 0 )
			low = mid + 1;
		else
			high = mid - 1;
	}
	cmpResult = compare( ele, &splitters[low] );

	if ( cmpResult > 0 && low < length)
		return (low + 1);
	return low;
}

/**
* @brief Chooses n-1 equidistant elements of the input data array as splitters
*
* @param[in] 	array 				Data from which extract splitters
* @param[in] 	length 				Length of the data array
* @param[in] 	n 					Number of parts in which to split data
* @param[out] 	newSplitters	 	Chosen splitters
*/
void chooseSplitters( int *array, const int length, const int n, int *newSplitters )
{
	int i, j, k;

	if ( length >= n )
		/* Choosing splitters (n-1 equidistant elements of the data array) */
		for ( i=0, k=j=length/n; i<n-1; i++, k+=j )
			newSplitters[i] = array[k];
	else {
		/* Choosing n-1 random splitters */
		for ( i=0; i<n-1; i++ )
			newSplitters[i] = array[rand()%length];
		qsort( newSplitters, n-1, sizeof(int), compare );
	}
}

/**
* @brief Chooses n-1 equidistant elements of the input data object as splitters
*
* @param[in] 	data 				Data from which extract splitters
* @param[in] 	n 					Number of parts in which to split data
* @param[out] 	newSplitters	 	Chosen splitters
*/
void chooseSplittersFromData( Data *data, const int n, int *newSplitters )
{
	/* TODO: Implement it the right way!! */

	chooseSplitters( data->array.data, data->array.size, n, newSplitters );
}

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
}
#endif

#endif
