
/**
* @file lbkmergesort.c
*
* @brief This file contains a set of functions used to sort atomic elements (integers) by using a parallel version of Multi-way Mergesort with load balancing
*
* @author Nicola Desogus
* @version 0.0.01
*/

#include <assert.h>
#include <string.h>
#include <queue>
#include <mpi.h>
#include "../sorting.h"
#include "../utils.h"

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
* @param[in] 	runs			Sequences to be merged
* @param[in] 	k				Number of sequences
* @param[in] 	lengths			Array containing the length of each run
* @param[in] 	displs			Displacements of each run
* @param[in] 	mergedLength    Length of the final merged sequence
* @param[out] 	mergedData		The merged sequence
*/
void kmerge( Data *runs, int k, long* lengths, long* displs, long mergedLength, Data *mergedData )
{
    /* TODO: Implement it the right way!! */

    std::priority_queue<Min_val> heap;
    int *runs_indexes = (int*) calloc( sizeof(int), k );
    int i;

    assert( DAL_reallocArray( mergedData, mergedLength ) );

    /* Initializing the heap */
    for ( i=0; i<k; i++ ) {
        if ( lengths[i] )
            heap.push( Min_val( runs->array.data[displs[i]], i ) );
    }
    /* Merging the runs */
    for ( i=0; i<mergedLength; i++ ) {
        Min_val min = heap.top();
        heap.pop();
        mergedData->array.data[i] = min.val;

        if ( ++(runs_indexes[min.run_index]) != lengths[min.run_index] )
            heap.push( Min_val( runs->array.data[displs[min.run_index] + runs_indexes[min.run_index]], min.run_index ) );
    }
    free( runs_indexes );
}


/**
* @brief Gets the number of element to be inserted in each small bucket
*
* @param[in] data       	The data object containing data to be distributed
* @param[in] splitters  	The array of splitters
* @param[in] n     			The number of buckets
* @param[out] lengths    	The array that will contain the small bucket lengths
*/
void getSendCounts( Data *data, const int *splitters, const int n, long *lengths )
{
    /* TODO: Implement it the right way!! */

    int i, j;

    /* Computing the number of integers to be sent to each process */
    for ( i=0; i<data->array.size; i++ ) {
        j = getBucketIndex( &data->array.data[i], splitters, n-1 );
        lengths[j]++;
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
	const int	root = 0;                          	//Rank (ID) of the root process
	const int	id = GET_ID( ti );                	//Rank (ID) of the process
	const int	n = GET_N( ti );                   	//Number of processes
	const long	M = GET_M( ti );                   	//Number of data elements
	const long	maxLocal_M = M / n + (0 < M%n);    	//Max number of elements assigned to a process
	const int	buffSize = 4*maxLocal_M;			//Max number of received elements (using the sampling technique, it is shown that each process will have, at the end, a maximum of 4*M/n elements to sort)

	Data		recvData;
	long 		dataLength = GET_LOCAL_M( ti );     //Number of elements assigned to this process

	long 		sendCounts[n], recvCounts[n];		//Number of elements in send/receive buffers
	long		sdispls[n], rdispls[n];				//Send/receive buffer displacements

	MPI_Status 	status;

	int			localSplitters[n-1];               	//Local splitters (n-1 equidistant elements of the data array)
	int			*allSplitters = 0;                 	//All splitters (will include all the local splitters)
	int			*globalSplitters = 0;            	//Global splitters (will be selected from the allSplitters array)

	long		i, j, k, h, z, flag;
	PhaseHandle scatterP, localP, samplingP, multiwayMergeP, gatherP;
	int 		groupSize, idInGroup, partner, pairedGroupRoot, groupRoot;

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
	DAL_scatterv( ti, data, sendCounts, sdispls, root );

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
	if ( id == root )
		allSplitters = (int*) malloc ( ((n-1)*n) * sizeof(int) );
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
/************************************* Parallel Multiway Merge Phase *******************************************/
/***************************************************************************************************************/

	multiwayMergeP = startPhase( ti, "parallel multiway merge" );

	/* Initializing the sendCounts array */
	memset( sendCounts, 0, n*sizeof(long) );

	/* Computing the number of integers to be sent to each process */
	getSendCounts( data, globalSplitters, n, sendCounts );

	/* Computing the displacements */
	for ( k=0, j=0; j<n; j++ ) {
		/* Computing the displacements relative to localData from which to take the outgoing data destined for each process */
		sdispls[j] = k;
		k += sendCounts[j];
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
			k = DAL_sendrecv( ti, data, sendCounts[j], sdispls[j], &recvData, buffSize, dataLength, j );
			dataLength += k;
			recvCounts[z++] = k;

			j = ((id + h*flag) % groupSize + groupRoot) ^ groupSize;		//Selects the next partner to avoid deadlocks
		}
	}
	k = DAL_sendrecv( ti, data, sendCounts[id], sdispls[id], &recvData, sendCounts[id], dataLength, id );
	dataLength += k;
	recvCounts[z] = k;

	/* Computing the displacements */
	for ( k=0, i=0; i<n; i++ ) {
		rdispls[i] = k;
		k += recvCounts[i];
	}
	/* Merging received data */
	kmerge ( &recvData, n, recvCounts, rdispls, dataLength, data );

	stopPhase( ti, multiwayMergeP );
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
	DAL_gatherv( ti, data, recvCounts, rdispls, root );

	stopPhase( ti, gatherP );
/*--------------------------------------------------------------------------------------------------------------*/

	/* Freeing memory */
	DAL_destroy( &recvData );
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
	}
}
