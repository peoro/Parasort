
/**
* @file bitonicsort.c
*
* @brief This file contains a set of functions used to sort atomic elements (integers) by using a parallel version of the Bitonic Sort
*
* @author Nicola Desogus
* @version 0.0.01
*/

#include <string.h>
#include "../sorting.h"
#include "../common.h"
#include "../dal_internals.h"

#define MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define MAX(a,b) ( (a)>(b) ? (a) : (b) )

/**
* @brief Compare and Exchange operation on the low part of two data objects with integers sorted in ascending order
*
* @param[in] 	d1			The first data object
* @param[out] 	d2			The second data object that will also contain the merging result
*/
void compareLowData( Data *d1, Data *d2 )
{
	Data merged;
	DAL_init( &merged );
	SPD_ASSERT( DAL_allocData( &merged, DAL_dataSize(d1) ), "not enough memory..." );

	/* Memory buffer */
	Data buffer;
	DAL_init( &buffer );
	SPD_ASSERT( DAL_allocBuffer( &buffer, DAL_allowedBufSize() ), "not enough memory..." );
	DAL_ASSERT( DAL_dataSize( &buffer ) > 2, &buffer, "buffer size must be greater than 2" );

	int bufSize = DAL_dataSize( &buffer ) / 3;
	dal_size_t d1Count, d2Count, mergedCount;
	dal_size_t d1c, d2c, mc;

	d1c = d2c = mc = MIN(bufSize, DAL_dataSize(d1));
	mergedCount = 0;

	d1Count = DAL_dataCopyOS( d1, 0, &buffer, 0, d1c );
	d2Count = DAL_dataCopyOS( d2, 0, &buffer, bufSize, d2c );

	int* d1Array = buffer.array.data;
	int* d2Array = buffer.array.data+bufSize;
	int* mergedArray = buffer.array.data+2*bufSize;
	int i, j, k;
	k = 0; j = 0; i = 0;

	while ( mergedCount < DAL_dataSize(d1) ) {
		while ( i < mc && j < d2c && k < d1c )
			if ( d2Array[j] <= d1Array[k] )
				mergedArray[i++] = d2Array[j++];
			else
				mergedArray[i++] = d1Array[k++];

		if ( i == mc ) {
			mergedCount += DAL_dataCopyOS( &buffer, 2*bufSize, &merged, mergedCount, mc );
			mc = MIN(bufSize, DAL_dataSize(&merged)-mergedCount);
			i = 0;
		}

		if ( j == d2c ) {
			d2c = MIN(bufSize, DAL_dataSize(d2)-d2Count);

			if ( d2c )
				d2Count += DAL_dataCopyOS( d2, d2Count, &buffer, bufSize, d2c );
			j = 0;
		}

		if ( k == d1c ) {
			d1c = MIN(bufSize, DAL_dataSize(d1)-d1Count);

			if ( d1c )
				d1Count += DAL_dataCopyOS( d1, d1Count, &buffer, 0, d1c );
			k = 0;
		}
	}
	DAL_destroy( &buffer );
	DAL_destroy( d2 );
	*d2 = merged;
}


/**
* @brief Compare and Exchange operation on the high part of two data objects with integers sorted in ascending order
*
* @param[in] 	d1			The first data object
* @param[out] 	d2			The second data object that will also contain the merging result
*/
void compareHighData( Data *d1, Data *d2 )
{
	Data merged;
	DAL_init( &merged );

	SPD_ASSERT( DAL_allocData( &merged, DAL_dataSize(d1) ), "not enough memory..." );

	/* Memory buffer */
	Data buffer;
	DAL_init( &buffer );
	SPD_ASSERT( DAL_allocBuffer( &buffer, DAL_allowedBufSize() ), "not enough memory..." );
	DAL_ASSERT( DAL_dataSize( &buffer ) > 2, &buffer, "buffer size must be greater than 2" );

	int bufSize = DAL_dataSize( &buffer ) / 3;
	dal_size_t d1Count, d2Count, mergedCount;
	dal_size_t d1Displ, d2Displ, mergedDispl;
	dal_size_t d1c, d2c, mc;

	d1c = d2c = mc = MIN(bufSize, DAL_dataSize(d1));
	mergedDispl = d1Displ = d2Displ = DAL_dataSize(d1)-d1c;
	mergedCount = d1Count = d2Count = 0;

	d1Count = DAL_dataCopyOS( d1, d1Displ, &buffer, 0, d1c );
	d2Count = DAL_dataCopyOS( d2, d2Displ, &buffer, bufSize, d2c );

	int* d1Array = buffer.array.data;
	int* d2Array = buffer.array.data+bufSize;
	int* mergedArray = buffer.array.data+2*bufSize;
	int i, j, k;
	k = j = i = d1c-1;

	while ( mergedCount < DAL_dataSize(d1) ) {
		while ( i >= 0 && j >= 0 && k >= 0 ) {
			if ( d2Array[j] >= d1Array[k] )
				mergedArray[i--] = d2Array[j--];
			else
				mergedArray[i--] = d1Array[k--];
		}

		if ( i < 0 ) {
			mergedCount += DAL_dataCopyOS( &buffer, 2*bufSize, &merged, mergedDispl, mc );
			mc = MIN(bufSize, DAL_dataSize(&merged)-mergedCount);
			mergedDispl -= mc;
			i = mc-1;
		}

		if ( j < 0 ) {
			d2c = MIN(bufSize, DAL_dataSize(d2)-d2Count);
			d2Displ -= d2c;

			if ( d2c )
				d2Count += DAL_dataCopyOS( d2, d2Displ, &buffer, bufSize, d2c );
			j = d2c-1;
		}

		if ( k < 0 ) {
			d1c = MIN(bufSize, DAL_dataSize(d1)-d1Count);
			d1Displ -= d1c;

			if ( d1c )
				d1Count += DAL_dataCopyOS( d1, d1Displ, &buffer, 0, d1c );
			k = d1c-1;
		}
	}
	DAL_destroy( &buffer );
	DAL_destroy( d2 );
	*d2 = merged;
}

/**
* @brief Sorts input data by using a parallel version of Bitonic Sort
*
* @param[in] ti        The TestInfo Structure
* @param[in] data      Data to be sorted
*/
void bitonicSort( const TestInfo *ti, Data *data )
{
	const int			root = 0;                           //Rank (ID) of the root process
	const int			id = GET_ID( ti );                  //Rank (ID) of the process
	const int			n = GET_N( ti );                    //Number of processes
	const dal_size_t	M = GET_M( ti );                    //Number of data elements
	const dal_size_t	local_M = GET_LOCAL_M( ti );        //Number of elements assigned to each process

	Data				recvData;

	int					mask, mask2, partner;
	int					i, j, k, z, flag;

	PhaseHandle 		scatterP, sortingP, computationP, localP, mergeP, gatherP;

	SPD_ASSERT( isPowerOfTwo( n ), "n should be a power of two (but it's %d)", n );

/***************************************************************************************************************/
/********************************************* Scatter Phase ***************************************************/
/***************************************************************************************************************/
	scatterP = startPhase( ti, "scattering" );

	/* Scattering data */
	DAL_scatter( data, local_M, root );

	stopPhase( ti, scatterP );
/*--------------------------------------------------------------------------------------------------------------*/

	sortingP = startPhase( ti, "sorting" );
	computationP = startPhase( ti, "computation" );

/***************************************************************************************************************/
/*********************************************** Local Phase ***************************************************/
/***************************************************************************************************************/
	localP = startPhase( ti, "sequential sort" );

	/* Sorting local data */
	sequentialSort( ti, data );

	stopPhase( ti, localP );
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/****************************************** Parallel Merge Phase ***********************************************/
/***************************************************************************************************************/

	mergeP = startPhase( ti, "parallel merge" );

	k = _log2( n );		//Number of steps of the outer loop
			
	/* Bitonic Sort */
	for ( i=1, mask=2; i<=k; i++, mask<<=1 ) {
		mask2 = 1 << (i - 1);					//Bitmask for partner selection
		flag = (id & mask) == 0 ? -1 : 1;		//flag=-1 iff id has a 0 at the i-th bit, flag=1 otherwise (NOTE: mask = 2^i)		
		
		for ( j=0; j<i; j++, mask2>>=1 ) {
			partner = id ^ mask2;				//Selects as partner the process with rank that differs from id only at the j-th bit
			
			DAL_init( &recvData );
			
			stopPhase( ti, computationP );			
			DAL_sendrecv( data, local_M, 0, &recvData, GET_LOCAL_M(ti), 0, partner );		//Exchanging data with partner
			resumePhase( ti, computationP );

			/* Each process must call the dual function of its partner */
			if ( (id-partner) * flag > 0 )
				compareLowData( &recvData, data );
			else
				compareHighData( &recvData, data );
			
			DAL_destroy(&recvData);
		}
	}				

	stopPhase( ti, mergeP );
/*--------------------------------------------------------------------------------------------------------------*/

	stopPhase( ti, computationP );
	stopPhase( ti, sortingP );

/***************************************************************************************************************/
/********************************************** Ghater Phase ***************************************************/
/***************************************************************************************************************/
	gatherP = startPhase( ti, "gathering" );

	/* Gathering sorted data */
	DAL_gather( data, local_M, root );

	stopPhase( ti, gatherP );
/*--------------------------------------------------------------------------------------------------------------*/
}

void mainSort( const TestInfo *ti, Data *data )
{
	bitonicSort( ti, data );
}

void sort( const TestInfo *ti )
{
	Data data;
	DAL_init( &data );
	bitonicSort( ti, &data );
	DAL_destroy( &data );
}
