
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
#include "../utils.h"

/**
* @brief Compare and Exchange operation on the low part of two data objects with integers sorted in ascending order
*
* @param[in] 	d1			The first data object
* @param[out] 	d2			The second data object that will also contain the merging result
*/
void compareLow( Data *d1, Data *d2 )
{
	/* TODO: Implement it the right way!! */

	int i, j, k;
	Data d3;
	DAL_init( &d3 );
	SPD_ASSERT( DAL_allocArray( &d3, d2->array.size ), "not enough memory..." );

	for ( i=j=k=0; i<d2->array.size; i++ )
		if ( d2->array.data[j] <= d1->array.data[k] )
			d3.array.data[i] = d2->array.data[j++];
		else
			d3.array.data[i] = d1->array.data[k++];

	free( d2->array.data );
	d2->array = d3.array;
}


/**
* @brief Compare and Exchange operation on the high part of two data objects with integers sorted in ascending order
*
* @param[in] 	d1			The first data object
* @param[out] 	d2			The second data object that will also contain the merging result
*/
void compareHigh( Data *d1, Data *d2 )
{
	/* TODO: Implement it the right way!! */

	int i, j, k;
	Data d3;
	DAL_init( &d3 );
	SPD_ASSERT( DAL_allocArray( &d3, d2->array.size ), "not enough memory..." );

	for ( i=j=k=d2->array.size-1; i>=0; i-- )
		if ( d2->array.data[j] >= d1->array.data[k] )
			d3.array.data[i] = d2->array.data[j--];
		else
			d3.array.data[i] = d1->array.data[k--];

	free( d2->array.data );
	d2->array = d3.array;
}

/**
* @brief Sorts input data by using a parallel version of Bitonic Sort
*
* @param[in] ti        The TestInfo Structure
* @param[in] data      Data to be sorted
*/
void bitonicSort( const TestInfo *ti, Data *data )
{
	const int		root = 0;                           //Rank (ID) of the root process
	const int		id = GET_ID( ti );                  //Rank (ID) of the process
	const int		n = GET_N( ti );                    //Number of processes
	const long		M = GET_M( ti );                    //Number of data elements
	const long		local_M = GET_LOCAL_M( ti );        //Number of elements assigned to each process

	Data			recvData;
	long 			recvCount = local_M;				//Number of elements that will be received from partner

	int				mask, mask2, partner;
	long			i, j, k, z, flag;

	PhaseHandle 	scatterP, localP, mergeP, gatherP;

	SPD_ASSERT( isPowerOfTwo( n ), "n should be a power of two (but it's %d)", n );

	/* Initializing data objects */
	DAL_init( &recvData );

/***************************************************************************************************************/
/********************************************* Scatter Phase ***************************************************/
/***************************************************************************************************************/
	scatterP = startPhase( ti, "scattering" );

	/* Scattering data */
	DAL_scatter( data, local_M, root );

	stopPhase( ti, scatterP );
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/*********************************************** Local Phase ***************************************************/
/***************************************************************************************************************/
	localP = startPhase( ti, "local sorting" );

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

			/* Exchanging data with partner */
			DAL_sendrecv( data, local_M, 0, &recvData, recvCount, 0, partner );

			/* Each process must call the dual function of its partner */
			if ( (id-partner) * flag > 0 )
				compareLow( &recvData, data );
			else
				compareHigh( &recvData, data );
		}
	}

	stopPhase( ti, mergeP );
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/********************************************** Ghater Phase ***************************************************/
/***************************************************************************************************************/
	gatherP = startPhase( ti, "gathering" );

	/* Gathering sorted data */
	DAL_gather( data, M, root );

	stopPhase( ti, gatherP );
/*--------------------------------------------------------------------------------------------------------------*/

	/* Freeing memory */
	DAL_destroy( &recvData );
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
