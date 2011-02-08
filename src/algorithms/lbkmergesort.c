
/**
* @file lbkmergesort.c
*
* @brief This file contains a set of functions used to sort atomic elements (integers) by using a parallel version of Multi-way Mergesort with load balancing
*
* @author Nicola Desogus
* @version 0.0.01
*/

#include <string.h>
#include <queue>
#include <mpi.h>
#include "../sorting.h"
#include "../common.h"
#include "../dal_internals.h"

#ifndef SPD_PIANOSA
	#define MPI_DST MPI_LONG
#else
	#define MPI_DST MPI_LONG_LONG
#endif

#define MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define MAX(a,b) ( (a)>(b) ? (a) : (b) )

struct Min_val {
	Min_val( int val, int run_index ) {
		this->val = val;
		this->run_index = run_index;
	}
	int val;
	int run_index;
	bool operator<( const Min_val& m ) const {
		return val > m.val;
	}
};

/**
* @brief Merges k sorted sequences
*
* @param[in]    runs			Sequences to be merged
* @param[in]    k               Number of sequences
* @param[out]   mergedSeq       The merged sequence
*/
void kmerge( Data *runs, int k, Data *mergedSeq )
{
    int i;
    std::priority_queue<Min_val> heap;
    int *runs_indexes = (int*) calloc( sizeof(int), k );
    SPD_ASSERT( runs_indexes != NULL, "not enough memory..." );

    /* Initializing the heap */
    for ( i=0; i<k; i++ ) {
        if ( runs[i].array.size )
            heap.push( Min_val( runs[i].array.data[0], i ) );
    }
    /* Merging the runs */
    for ( i=0; i<mergedSeq->array.size; i++ ) {
        Min_val min = heap.top();
        heap.pop();
        mergedSeq->array.data[i] = min.val;

        if ( ++(runs_indexes[min.run_index]) != runs[min.run_index].array.size )
            heap.push( Min_val( runs[min.run_index].array.data[runs_indexes[min.run_index]], min.run_index ) );
    }
    free( runs_indexes );
}


/**
* @brief Merges k sorted sequences of ingers stored into a data object
*
* @param[in] 	runs			Sequences to be merged
* @param[in] 	k				Number of sequences
* @param[in] 	mergedLength    Length of the final merged sequence
* @param[out] 	mergedData		The merged sequence
*/
void kmergeData( Data *runs, int k, dal_size_t mergedLength, Data *mergedData )
{
	SPD_ASSERT( DAL_reallocData( mergedData, mergedLength ), "not enough memory..." );

	int i, type = mergedData->medium;
	for( i=0; i<k; i++ )
		switch ( runs[i].medium ) {
			case File: {
				type = File;
				break;
			}
			case Array: {
				break;
			}
			default:
				DAL_UNSUPPORTED( &runs[i] );
				break;
		}

	switch( type ) {
		case File: {
			fileKMerge( runs, k, mergedData );
			break;
		}
		case Array: {
			kmerge( runs, k, mergedData );
			break;
		}
	}
}


/**
* @brief Gets the number of element to be inserted in each small bucket
*
* @param[in] data       	The data object containing data to be distributed
* @param[in] splitters  	The array of splitters
* @param[in] n     			The number of buckets
* @param[out] lengths    	The array that will contain the small bucket lengths
*/
void getSendCounts( Data *data, const int *splitters, const int n, dal_size_t *lengths )
{
	int i, j, k;

	/* Initializing the lengths array */
	memset( lengths, 0, n*sizeof(dal_size_t) );

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
					lengths[j]++;
				}
			}
			SPD_ASSERT( readCount == DAL_dataSize(data), DST" elements have been read, while Data size is "DST, readCount, DAL_dataSize(data) );

			DAL_destroy( &buffer );
			break;
		}
		case Array: {
			for ( i=0; i<data->array.size; i++ ) {
				j = getBucketIndex( &data->array.data[i], splitters, n-1 );
				lengths[j]++;
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
void lbkmergesort( const TestInfo *ti, Data *data )
{
	const int			root = 0;                          	//Rank (ID) of the root process
	const int			id = GET_ID( ti );                	//Rank (ID) of the process
	const int			n = GET_N( ti );                   	//Number of processes
	const dal_size_t	M = GET_M( ti );                   	//Number of data elements
	dal_size_t 			dataLength = GET_LOCAL_M( ti );     //Number of elements assigned to this process

	dal_size_t 			sendCounts[n], recvCounts[n];		//Number of elements in send/receive buffers
	dal_size_t			sdispls[n], rdispls[n];				//Send/receive buffer displacements

	MPI_Status 			status;

	int					localSplitters[n-1];               	//Local splitters (n-1 equidistant elements of the data array)
	int					*allSplitters = 0;                 	//All splitters (will include all the local splitters)
	int					*globalSplitters = 0;            	//Global splitters (will be selected from the allSplitters array)

	dal_size_t			i, j, k, h, z, flag;
	PhaseHandle 		scatterP, sortingP, computationP, localP, samplingP, multiwayMergeP, gatherP;
	int 				groupSize, idInGroup, partner, pairedGroupRoot, groupRoot;

	SPD_ASSERT( isPowerOfTwo( n ), "n should be a power of two (but it's %d)", n );

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
/************************************* Parallel Multiway Merge Phase *******************************************/
/***************************************************************************************************************/

	multiwayMergeP = startPhase( ti, "parallel multiway merge" );

	/* Initializing the sendCounts array */
	memset( sendCounts, 0, n*sizeof(dal_size_t) );

	/* Computing the number of integers to be sent to each process */
	getSendCounts( data, globalSplitters, n, sendCounts );

	/* Computing the displacements and initializing recvData */
	Data recvData[n];
	for ( k=0, j=0; j<n; j++ ) {
		/* Computing the displacements relative to localData from which to take the outgoing data destined for each process */
		sdispls[j] = k;
		k += sendCounts[j];

		DAL_init( &recvData[j] );
	}

	for ( dataLength=0, z=0, i=1, groupSize=1; i<=_log2( n ); i++, groupSize<<=1 ) {
		partner = id ^ groupSize;		//Selects as partner the process with rank that differs from id only at the i-th bit
		idInGroup = id % groupSize;		//Retrieves the id of the process within the group

		/* Retrieving the id of the group root and the paired group root */
		groupRoot = id-idInGroup;
		pairedGroupRoot = (id&groupSize) ? groupRoot-groupSize : groupRoot+groupSize;

		flag = groupRoot < pairedGroupRoot ? 1 : -1;

		/* Exchanging data with the paired group avoiding deadlocks */
		for ( h=1, j=partner; h<=groupSize; h++ ) {
			stopPhase( ti, computationP );
			k = DAL_sendrecv( data, sendCounts[j], sdispls[j], &recvData[j], 0, j );
			resumePhase( ti, computationP );
			dataLength += k;
			recvCounts[z++] = k;

			j = ((id + h*flag) % groupSize + groupRoot) ^ groupSize;		//Selects the next partner to avoid deadlocks
		}
	}
	DAL_allocData( &recvData[id], sendCounts[id] );
	DAL_dataCopyOS( data, sdispls[id], &recvData[id], 0, sendCounts[id] );	
	dataLength += sendCounts[id];
	recvCounts[z] = sendCounts[id];

	/* Merging received data */
	kmergeData( recvData, n, dataLength, data );

	stopPhase( ti, multiwayMergeP );
/*--------------------------------------------------------------------------------------------------------------*/

	stopPhase( ti, computationP );
	stopPhase( ti, sortingP );

/***************************************************************************************************************/
/********************************************** Ghater Phase ***************************************************/
/***************************************************************************************************************/
	gatherP = startPhase( ti, "gathering" );

	/* Gathering the lengths of the all buckets */
	MPI_Gather( &dataLength, 1, MPI_DST, recvCounts, 1, MPI_DST, root, MPI_COMM_WORLD );

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

	/* Freeing memory */
	for ( i=0; i<n; i++ )
		DAL_destroy( &recvData[i] );
}

extern "C"
{
	void mainSort( const TestInfo *ti, Data *data )
	{
		lbkmergesort( ti, data );
	}

	void sort( const TestInfo *ti )
	{
		Data data;
		DAL_init( &data );
		lbkmergesort( ti, &data );
		DAL_destroy( &data );
	}
}
