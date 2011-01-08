
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

/**
* @brief Compare and Exchange operation on the low part of two sequences sorted in ascending order
*
* @param[in]    length                  The length of the sequences
* @param[in]    firstSeq                The first sequence
* @param[in]    secondSeq       The second sequence
* @param[out]   mergedSeq               The merged sequence
*/
void compareLow( const long length, int *firstSeq, int *secondSeq, int* mergedSeq )
{
    int i, j, k;

    for ( i=j=k=0; i<length; i++ )
        if ( secondSeq[j] <= firstSeq[k] )
            mergedSeq[i] = secondSeq[j++];
        else
            mergedSeq[i] = firstSeq[k++];
}


/**
* @brief Compare and Exchange operation on the high part of two sequences sorted in ascending order
*
* @param[in]    length                  The length of the sequences
* @param[in]    firstSeq                The first sequence
* @param[in]    secondSeq       The second sequence
* @param[out]   mergedSeq               The merged sequence
*/
void compareHigh( const long length, int *firstSeq, int *secondSeq, int* mergedSeq )
{
    int i, j, k;

    for ( i=j=k=length-1; i>=0; i-- )
        if ( secondSeq[j] >= firstSeq[k] )
            mergedSeq[i] = secondSeq[j--];
        else
            mergedSeq[i] = firstSeq[k--];
}


/**
* @brief Compare and Exchange operation on the low part of two data objects with integers sorted in ascending order
*
* @param[in] 	d1			The first data object
* @param[out] 	d2			The second data object that will also contain the merging result
*/
void compareLowData( Data *d1, Data *d2 )
{
	/* TODO: Implement it the right way!! */
	Data buffer;
	DAL_init( &buffer );

	switch( d1->medium ) {
		case File: {
			DAL_UNIMPLEMENTED( d1 );
			break;
		}
		case Array: {
			SPD_ASSERT( DAL_allocArray( &buffer, d1->array.size ), "not enough memory..." );
			compareLow( d1->array.size, d1->array.data, d2->array.data, buffer.array.data );
			break;
		}
		default:
			DAL_UNSUPPORTED( d1 );
	}
	DAL_destroy( d2 );
	*d2 = buffer;
}


/**
* @brief Compare and Exchange operation on the high part of two data objects with integers sorted in ascending order
*
* @param[in] 	d1			The first data object
* @param[out] 	d2			The second data object that will also contain the merging result
*/
void compareHighData( Data *d1, Data *d2 )
{
	/* TODO: Implement it the right way!! */

	int i, j, k;
	Data buffer;
	DAL_init( &buffer );

	switch( d1->medium ) {
		case File: {
			DAL_UNIMPLEMENTED( d1 );
			break;
		}
		case Array: {
			SPD_ASSERT( DAL_allocArray( &buffer, d1->array.size ), "not enough memory..." );
			compareHigh( d1->array.size, d1->array.data, d2->array.data, buffer.array.data );
			break;
		}
		default:
			DAL_UNSUPPORTED( d1 );
	}
	DAL_destroy( d2 );
	*d2 = buffer;
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
				compareLowData( &recvData, data );
			else
				compareHighData( &recvData, data );
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
