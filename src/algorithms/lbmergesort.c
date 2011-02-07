
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

#define MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define MAX(a,b) ( (a)>(b) ? (a) : (b) )

/**
* @brief Merges two sorted sequences
*
* @param[in]    flength         The length of the first sequence
* @param[in]    firstSeq        The first sequence
* @param[in]    slength         The length of the second sequence
* @param[in]    secondSeq       The second sequence
* @param[out]   mergedSeq       The merged sequence
*/
void merge( const dal_size_t flength, int *firstSeq, const dal_size_t slength, int *secondSeq, int *mergedSeq )
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
* @brief Merges two sorted sequences
*
* @param[in]    flength         The length of the first sequence
* @param[in]    fdispl          The displacement for the first sequence
* @param[in]    firstSeq        The first sequence
* @param[in]    slength         The length of the second sequence
* @param[in]    sdispl          The displacement for the second sequence
* @param[in]    secondSeq       The second sequence
* @param[out]   mergedSeq       The merged sequence
*/
void fileMerge( const dal_size_t flength, const dal_size_t fdispl, Data *d1, const dal_size_t slength, const dal_size_t sdispl, Data *d2, Data *merged )
{
	/* Memory buffer */
	Data buffer;
	DAL_init( &buffer );
	SPD_ASSERT( DAL_allocBuffer( &buffer, DAL_allowedBufSize() ), "not enough memory..." );
	DAL_ASSERT( DAL_dataSize( &buffer ) > 2, &buffer, "buffer size must be greater than 2" );

	int bufSize = DAL_dataSize( &buffer ) / 3;
	dal_size_t d1Count, d2Count, mergedCount;
	dal_size_t d1c, d2c, mc;

	d1c = MIN(bufSize, flength);
	d2c = MIN(bufSize, slength);
	mc = MIN(bufSize, DAL_dataSize(merged));
	d1Count = d2Count = mergedCount = 0;

	DAL_dataCopyOS( d1, fdispl, &buffer, 0, d1c );
	DAL_dataCopyOS( d2, sdispl, &buffer, bufSize, d2c );

	int* d1Array = buffer.array.data;
	int* d2Array = buffer.array.data+bufSize;
	int* mergedArray = buffer.array.data+2*bufSize;
	int i, j, k;
	k = 0; j = 0; i = 0;

	while ( d1Count < flength && d2Count < slength ) {
		while ( i < mc && j < d2c && k < d1c )
			if ( d2Array[j] <= d1Array[k] )
				mergedArray[i++] = d2Array[j++];
			else
				mergedArray[i++] = d1Array[k++];

		mergedCount += DAL_dataCopyOS( &buffer, 2*bufSize, merged, mergedCount, i );
		mc = MIN(bufSize, DAL_dataSize(merged)-mergedCount);
		i = 0;

		if ( j == d2c ) {
			d2Count += d2c;
			d2c = MIN(bufSize, slength-d2Count);

			if ( d2c )
				DAL_dataCopyOS( d2, sdispl+d2Count, &buffer, bufSize, d2c );
			j = 0;
		}

		if ( k == d1c ) {
			d1Count += d1c;
			d1c = MIN(bufSize, flength-d1Count);

			if ( d1c )
				DAL_dataCopyOS( d1, fdispl+d1Count, &buffer, 0, d1c );
			k = 0;
		}
	}
	if ( d1Count < flength )
		DAL_dataCopyOS( d1, fdispl+d1Count+k, merged, mergedCount, DAL_dataSize(merged)-mergedCount );
	else
		DAL_dataCopyOS( d2, sdispl+d2Count+j, merged, mergedCount, DAL_dataSize(merged)-mergedCount );
	DAL_destroy( &buffer );
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
void mergeData( dal_size_t flength, dal_size_t fdispl, Data *d1, dal_size_t slength, dal_size_t sdispl, Data *d2 )
{
    Data merged;
    DAL_init( &merged );
	SPD_ASSERT( DAL_allocData( &merged, flength+slength ), "not enough memory..." );

	if( d1->medium == File || d2->medium == File || merged.medium == File ) {
		fileMerge( flength, fdispl, d1, slength, sdispl, d2, &merged );
	}
	else if ( d1->medium == Array && d2->medium == Array && merged.medium == Array ) {
		merge( flength, d1->array.data+fdispl, slength, d2->array.data+sdispl, merged.array.data );
	}
	else {
		Data *tmp = d1->medium != Array ? d1 : d2->medium != Array ? d2 : &merged;
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
void getSendCounts( Data *data, const int *splitters, const int n, const int start, dal_size_t *lengths )
{
	int i, j, k;

	/* Computing the number of integers to be sent to each process */
	switch( data->medium ) {
		case File: {
			/* Memory buffer */
			Data buffer;
			DAL_init( &buffer );
			SPD_ASSERT( DAL_allocBuffer( &buffer, DAL_dataSize(data) ), "not enough memory..." );

			int blocks = DAL_BLOCK_COUNT(data, &buffer);
			dal_size_t r, readCount = 0;
			int i;

			for( i=0; i<blocks; i++ ) {
				r = DAL_dataCopyOS( data, i*DAL_dataSize(&buffer), &buffer, 0, MIN(DAL_dataSize(&buffer), DAL_dataSize(data)-readCount) );
				readCount += r;

				for ( k=0; k<r; k++ ) {
					j = getBucketIndex( &buffer.array.data[k], splitters, n-1 );
					SPD_ASSERT( j >= 0 && j < n, "Something went wrong: j should be within [0,%d], but it's %d", n-1, j );
					lengths[start+j]++;
				}
			}
			SPD_ASSERT( readCount == DAL_dataSize(data), DST" elements have been read, while Data size is "DST, readCount, DAL_dataSize(data) );

			DAL_destroy( &buffer );
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
	const int			root = 0;                           	//Rank (ID) of the root process
	const int			id = GET_ID( ti );                  	//Rank (ID) of the process
	const int			n = GET_N( ti );                    	//Number of processes
	const dal_size_t	M = GET_M( ti );                    	//Number of data elements
	const dal_size_t	maxLocal_M = M / n + (0 < M%n);     	//Max number of elements assigned to a process
	const dal_size_t	buffSize = n > 4 ? (n>>1)*maxLocal_M : 2*maxLocal_M;

	dal_size_t			dataLength = GET_LOCAL_M( ti );         //Number of elements assigned to this process
	dal_size_t			recvDataLength, sentDataLength;

	dal_size_t 			sendCounts[n], recvCounts[n];			//Number of elements in send/receive buffers
	dal_size_t			sdispls[n], rdispls[n];					//Send/receive buffer displacements

	Data				recvData;

	int					localSplitters[n-1];               		//Local splitters (n-1 equidistant elements of the data array)
	int					*allSplitters = 0;                 		//All splitters (will include all the local splitters)
	int					*globalSplitters = 0;            		//Global splitters (will be selected from the allSplitters array)
	int					splitters[n-1];
	int 				splittersCount = 0;

	int 				groupSize, idInGroup, partner, pairedGroupRoot, groupRoot;
	int					i, j, k, h, flag;
	PhaseHandle 		scatterP, sortingP, computationP, localP, samplingP, mergeP, gatherP;

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
	stopPhase( ti, computationP );
	MPI_Gather( localSplitters, n-1, MPI_INT, allSplitters, n-1, MPI_INT, root, MPI_COMM_WORLD );
	resumePhase( ti, computationP );

	/* Choosing global splitters (n-1 equidistant elements of the allSplitters array) */
	globalSplitters = localSplitters;     //--> To save space but keeping the correct semantics!!! :F

	if ( id == root ) {
		qsort( allSplitters, (n-1)*n, sizeof(int), compare );
		chooseSplitters( allSplitters, (n-1)*n, n, globalSplitters );
	}
	stopPhase( ti, computationP );
	/* Broadcasting global splitters */
	MPI_Bcast( globalSplitters, n-1, MPI_INT, root, MPI_COMM_WORLD );
	resumePhase( ti, computationP );

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
		memset( sendCounts, 0, n*sizeof(dal_size_t) );

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
			stopPhase( ti, computationP );
			recvDataLength += DAL_sendrecv( data, sendCounts[j], sdispls[j], &recvData, buffSize, recvDataLength, j );
			resumePhase( ti, computationP );
			sentDataLength += sendCounts[j];

			j = ((id + h*flag) % groupSize + groupRoot) ^ groupSize;		//Selects the next partner to avoid deadlocks
		}
 		mergeData( recvDataLength, 0, &recvData, dataLength-sentDataLength, sdispls[groupRoot], data );
		dataLength = dataLength - sentDataLength + recvDataLength;		
	}

	stopPhase( ti, mergeP );
/*--------------------------------------------------------------------------------------------------------------*/

	stopPhase( ti, computationP );
	stopPhase( ti, sortingP );

/***************************************************************************************************************/
/********************************************** Ghater Phase ***************************************************/
/***************************************************************************************************************/
	gatherP = startPhase( ti, "gathering" );

	/* Gathering the lengths of the all buckets */
	MPI_Gather( &dataLength, 1, MPI_LONG_LONG, recvCounts, 1, MPI_LONG_LONG, root, MPI_COMM_WORLD );

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
