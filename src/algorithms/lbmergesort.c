
/**
* @file lbmergesort.c
*
* @brief This file contains a set of functions used to sort atomic elements (integers) by using a parallel version of Mergesort with load balancing
*
* @author Nicola Desogus
* @version 0.0.01
*/

#include <string.h>
#include <mpi.h>
#include "../sorting.h"
#include "../common.h"
#include "../dal_internals.h"

/**
* @brief Merges two sorted sequences
*
* @param[in]    flength         The length of the first sequence
* @param[in]    firstSeq        The first sequence
* @param[in]    slength         The length of the second sequence
* @param[in]    secondSeq       The second sequence
* @param[out]   mergedSeq       The merged sequence
*/
void merge( const long flength, int *firstSeq, const long slength, int *secondSeq, int* mergedSeq )
{
    int i, j, k;
    i = j = k = 0;

    while ( j < flength && k < slength ) {
        if ( firstSeq[j] < secondSeq[k] )
            mergedSeq[i++] = firstSeq[j++];
        else
            mergedSeq[i++] = secondSeq[k++];
    }
    while ( j<flength )
        mergedSeq[i++] = firstSeq[j++];

    while ( k<slength )
        mergedSeq[i++] = secondSeq[k++];
}


/**
* @brief Merges sorted integers stored into two data objects
*
* @param[in] 		flength			The number of elements to be merged from the first data object
* @param[in] 		fdispl			The displacement for the first data object
* @param[in] 		d1				The first data object
* @param[in] 		slength			The number of elements to be merged from the second data object
* @param[in] 		sdispl			The displacement for the second data object
* @param[in,out] 	d2				The second data object that will also contain the merging result
*/
void mergeData( long flength, long fdispl, Data *d1, long slength, long sdispl, Data *d2 )
{
    /* TODO: Implement it the right way!! */

    Data merged;
    DAL_init( &merged );

	if ( d1->medium == Array && d2->medium == Array ) {
		SPD_ASSERT( DAL_allocArray( &merged, flength+slength ), "not enough memory..." );
		merge( flength, d1->array.data+fdispl, slength, d2->array.data+sdispl, merged.array.data );
	}
	else if ( d1->medium == (Array || File) && d2->medium == (Array || File) ) {
		Data *tmp = d1->medium == File ? d1 : d2;
		DAL_UNIMPLEMENTED( tmp );
	}
	else {
		Data *tmp = d1->medium == File ? d1 : d2;
		DAL_UNSUPPORTED( tmp );
	}
	DAL_destroy( d2 );
	*d2 = merged;
}




/**
* @brief Gets the number of element to be inserted in each small bucket
*
* @param[in] data       	The data object containing data to be distributed
* @param[in] splitters  	The array of splitters
* @param[in] n     			The number of buckets
* @param[in] start			The index from which to start
* @param[out] lengths    	The array that will contain the small bucket lengths
*/
void getSendCounts( Data *data, const int *splitters, const int n, const int start, long *lengths )
{
	/* TODO: Implement it the right way!! */

	int i, j;

	/* Computing the number of integers to be sent to each process */
	switch( data->medium ) {
		case File: {
			DAL_UNIMPLEMENTED( data );
			break;
		}
		case Array: {
			for ( i=0; i<data->array.size; i++ ) {
				j = getBucketIndex( &data->array.data[i], splitters, n-1 );
				lengths[start+j]++;
			}
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}
}

/**
* @brief Sorts input data by using a parallel version of Mergesort with load balancing
*
* @param[in] ti        The TestInfo Structure
* @param[in] data      Data to be sorted
*/
void lbmergesort( const TestInfo *ti, Data *data )
{
	const int	root = 0;                           	//Rank (ID) of the root process
	const int	id = GET_ID( ti );                  	//Rank (ID) of the process
	const int	n = GET_N( ti );                    	//Number of processes
	const long	M = GET_M( ti );                    	//Number of data elements
	const long	maxLocal_M = M / n + (0 < M%n);     	//Max number of elements assigned to a process
	const long	buffSize = n > 4 ? (n>>1)*maxLocal_M : 2*maxLocal_M;

	long		dataLength = GET_LOCAL_M( ti );         //Number of elements assigned to this process
	long		recvDataLength, sentDataLength;

	long 		sendCounts[n], recvCounts[n];			//Number of elements in send/receive buffers
	long		sdispls[n], rdispls[n];					//Send/receive buffer displacements

	Data		recvData;

	int			localSplitters[n-1];               	//Local splitters (n-1 equidistant elements of the data array)
	int			*allSplitters = 0;                 	//All splitters (will include all the local splitters)
	int			*globalSplitters = 0;            	//Global splitters (will be selected from the allSplitters array)
	int			splitters[n-1];
	int 		splittersCount = 0;

	int 		groupSize, idInGroup, partner, pairedGroupRoot, groupRoot;
	int			i, j, k, h, flag;
	PhaseHandle scatterP, localP, samplingP, mergeP, gatherP;

	SPD_ASSERT( isPowerOfTwo( n ), "n should be a power of two (but it's %d)", n );

	/* Initializing data objects */
	DAL_init( &recvData );

/***************************************************************************************************************/
/********************************************* Scatter Phase ***************************************************/
/***************************************************************************************************************/
	scatterP = startPhase( ti, "scattering" );

	/* Computing the number of elements to be sent to each process and relative displacements */
	for ( k=0, i=0; i<n; i++ ) {
		sdispls[i] = k;
		sendCounts[i] = M/n + (i < M%n);
		k += sendCounts[i];
	}
	/* Scattering data */
	DAL_scatterv( data, sendCounts, sdispls, root );

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
/********************************************* Sampling Phase **************************************************/
/***************************************************************************************************************/

	samplingP = startPhase( ti, "sampling" );

	/* Choosing local splitters (n-1 equidistant elements of the data object) */
	chooseSplittersFromData( data, n, localSplitters );

	/* Gathering all splitters to the root process */
	if ( id == root ) {
		allSplitters = (int*) malloc ( ((n-1)*n) * sizeof(int) );
		SPD_ASSERT( allSplitters != NULL, "not enough memory..." );
	}
	MPI_Gather( localSplitters, n-1, MPI_INT, allSplitters, n-1, MPI_INT, root, MPI_COMM_WORLD );

	/* Choosing global splitters (n-1 equidistant elements of the allSplitters array) */
	globalSplitters = localSplitters;     //--> To save space but keeping the correct semantics!!! :F

	if ( id == root ) {
		qsort( allSplitters, (n-1)*n, sizeof(int), compare );
		chooseSplitters( allSplitters, (n-1)*n, n, globalSplitters );
	}
	/* Broadcasting global splitters */
	MPI_Bcast( globalSplitters, n-1, MPI_INT, root, MPI_COMM_WORLD );

	stopPhase( ti, samplingP );
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/****************************************** Parallel Merge Phase ***********************************************/
/***************************************************************************************************************/

	mergeP = startPhase( ti, "parallel merge" );

	for ( i=1, groupSize=1; i<=_log2( n ); i++, groupSize<<=1 ) {
		splittersCount += groupSize;	//Updates the number of splitters needed in this step
		partner = id ^ groupSize;		//Selects as partner the process with rank that differs from id only at the i-th bit
		idInGroup = id % groupSize;		//Retrieves the id of the process within the group

		/* Retrieving the id of the group root and the paired group root */
		groupRoot = id-idInGroup;
		pairedGroupRoot = (id&groupSize) ? groupRoot-groupSize : groupRoot+groupSize;

		/* Selecting the splitters for this step */
		k = groupRoot < pairedGroupRoot ? groupRoot : pairedGroupRoot;
		for ( j=0; j<splittersCount; j++ )
			splitters[j] = globalSplitters[k++];

		/* Initializing the sendCounts array */
		memset( sendCounts, 0, n*sizeof(long) );

		/* Computing the number of integers to be sent to each process of the paired group */
		h = id < partner ? groupRoot : pairedGroupRoot;
		getSendCounts( data, splitters, splittersCount+1, h, sendCounts );

		/* Computing the displacements */
		for ( k=0, j=0; j<n; j++ ) {
			/* Computing the displacements relative to localData from which to take the outgoing data destined for each process */
			sdispls[j] = k;
			k += sendCounts[j];
		}
		sentDataLength = recvDataLength = 0;
		flag = groupRoot < pairedGroupRoot ? 1 : -1;

		/* Exchanging data with the paired group avoiding deadlocks */
		for ( h=1, j=partner; h<=groupSize; h++ ) {
			recvDataLength += DAL_sendrecv( data, sendCounts[j], sdispls[j], &recvData, buffSize, recvDataLength, j );
			sentDataLength += sendCounts[j];

			j = ((id + h*flag) % groupSize + groupRoot) ^ groupSize;		//Selects the next partner to avoid deadlocks
		}
 		mergeData( recvDataLength, 0, &recvData, dataLength-sentDataLength, sdispls[groupRoot], data );
		dataLength = dataLength - sentDataLength + recvDataLength;
		DAL_destroy( &recvData );
	}

	stopPhase( ti, mergeP );
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/********************************************** Ghater Phase ***************************************************/
/***************************************************************************************************************/
	gatherP = startPhase( ti, "gathering" );

	/* Gathering the lengths of the all buckets */
	MPI_Gather( &dataLength, 1, MPI_LONG, recvCounts, 1, MPI_LONG, root, MPI_COMM_WORLD );

	/* Computing displacements relative to the output array at which to place the incoming data from each process  */
	if ( id == root ) {
		for ( k=0, i=0; i<n; i++ ) {
			rdispls[i] = k;
			k += recvCounts[i];
		}
		/* Freeing memory */
		free( allSplitters );
	}
	/* Gathering sorted data */
	DAL_gatherv( data, recvCounts, rdispls, root );

	stopPhase( ti, gatherP );
/*--------------------------------------------------------------------------------------------------------------*/

	DAL_destroy( &recvData );
}

void mainSort( const TestInfo *ti, Data *data )
{
	lbmergesort( ti, data );
}

void sort( const TestInfo *ti )
{
	Data data;
	DAL_init( &data );
	lbmergesort( ti, &data );
	DAL_destroy( &data );
}
