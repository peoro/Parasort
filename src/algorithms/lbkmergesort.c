
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
* @param[out] 	mergedSeq		The merged sequence
*/
void kmerge( int *runs, int k, int* lengths, int* displs, int mergedLength, int *mergedSeq )
{
	std::priority_queue<Min_val> heap;
	int *runs_indexes = (int*) calloc( sizeof(int), k );
	int i;

	/* Initializing the heap */
	for ( i=0; i<k; i++ ) {
		if ( lengths[i] )
			heap.push( Min_val( runs[displs[i]], i ) );
	}
	/* Merging the runs */
	for ( i=0; i<mergedLength; i++ ) {
		Min_val min = heap.top();
		heap.pop();
		mergedSeq[i] = min.val;

		if ( ++(runs_indexes[min.run_index]) != lengths[min.run_index] )
			heap.push( Min_val( runs[displs[min.run_index] + runs_indexes[min.run_index]], min.run_index ) );
	}
	free( runs_indexes );
}


/**
* @brief Sorts input data by using a parallel version of Mergesort with load balancing
*
* @param[in] ti        The TestInfo Structure
* @param[in] data      Data to be sorted
*/
void lbkmergesort( const TestInfo *ti, int *data )
{
	const int	root = 0;                          	//Rank (ID) of the root process
	const int	id = GET_ID( ti );                	//Rank (ID) of the process
	const int	n = GET_N( ti );                   	//Number of processes
	const long	M = GET_M( ti );                   	//Number of data elements
	const long	maxLocal_M = M / n + (0 < M%n);    	//Max number of elements assigned to a process
	const int	buffSize = 4*maxLocal_M;			//Max number of received elements (using the sampling technique, it is shown that each process will have, at the end, a maximum of 4*M/n elements to sort)

	int			*localData, *recvData;
	long 		dataLength = GET_LOCAL_M( ti );     //Number of elements assigned to this process

	int 		sendCounts[n], recvCounts[n];		//Number of elements in send/receive buffers
	int			sdispls[n], rdispls[n];				//Send/receive buffer displacements

	MPI_Status 	status;

	int			localSplitters[n-1];               	//Local splitters (n-1 equidistant elements of the data array)
	int			*allSplitters = 0;                 	//All splitters (will include all the local splitters)
	int			*globalSplitters = 0;            	//Global splitters (will be selected from the allSplitters array)

	int			i, j, k, h, z, flag;
	PhaseHandle scatterP, localP, samplingP, multiwayMergeP, gatherP;
	int 		groupSize, idInGroup, partner, pairedGroupRoot, groupRoot;

	/* Allocating memory */
	localData = (int*) malloc( buffSize * sizeof(int) );
	recvData = (int*) malloc( buffSize * sizeof(int) );

/***************************************************************************************************************/
/********************************************* Scatter Phase ***************************************************/
/***************************************************************************************************************/
	scatterP = startPhase( ti, "scattering" );

	/* Computing the number of elements to be sent to each process and relative displacements */
	if ( id == root ) {
		for ( k=0, i=0; i<n; i++ ) {
			sdispls[i] = k;
			sendCounts[i] = M/n + (i < M%n);
			k += sendCounts[i];
		}
	}
	/* Scattering data */
	MPI_Scatterv( data, sendCounts, sdispls, MPI_INT, localData, maxLocal_M, MPI_INT, root, MPI_COMM_WORLD );

	stopPhase( ti, scatterP );
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/*********************************************** Local Phase ***************************************************/
/***************************************************************************************************************/
	localP = startPhase( ti, "local sorting" );

	/* Sorting local data */
	qsort( localData, dataLength, sizeof(int), compare );

	stopPhase( ti, localP );
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/********************************************* Sampling Phase **************************************************/
/***************************************************************************************************************/

	samplingP = startPhase( ti, "sampling" );

	/* Choosing local splitters (n-1 equidistant elements of the data array) */
	chooseSplitters( localData, dataLength, n, localSplitters );

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
	memset( sendCounts, 0, n*sizeof(int) );

	/* Computing the number of integers to be sent to each process of the paired group */
	for ( j=0; j<dataLength; j++ ) {
		k = getBucketIndex( &localData[j], globalSplitters, n-1 );
		sendCounts[k]++;
	}

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
			MPI_Sendrecv( &localData[sdispls[j]], sendCounts[j], MPI_INT, j, 100, &recvData[dataLength], buffSize, MPI_INT, j, 100, MPI_COMM_WORLD, &status );

			MPI_Get_count( &status, MPI_INT, &k);
			dataLength += k;
			recvCounts[z++] = k;

			j = ((id + h*flag) % groupSize + groupRoot) ^ groupSize;		//Selects the next partner to avoid deadlocks
		}
	}
	/* Copying local section of data */
 	memcpy( &recvData[dataLength], &localData[sdispls[id]], sendCounts[id]*sizeof(int) );
 	recvCounts[z] = sendCounts[id];
 	dataLength += recvCounts[z];

	/* Computing the displacements */
	for ( k=0, i=0; i<n; i++ ) {
		rdispls[i] = k;
		k += recvCounts[i];
	}
	/* Merging received data */
	kmerge ( recvData, n, recvCounts, rdispls, dataLength, localData );

	stopPhase( ti, multiwayMergeP );
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/********************************************** Ghater Phase ***************************************************/
/***************************************************************************************************************/
	gatherP = startPhase( ti, "gathering" );

	/* Gathering the lengths of the all buckets */
	MPI_Gather( &dataLength, 1, MPI_INT, recvCounts, 1, MPI_INT, root, MPI_COMM_WORLD );

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
	MPI_Gatherv( localData, dataLength, MPI_INT, data, recvCounts, rdispls, MPI_INT, root, MPI_COMM_WORLD );

	stopPhase( ti, gatherP );
/*--------------------------------------------------------------------------------------------------------------*/

	/* Freeing memory */
	free( localData );
	free( recvData );
}

extern "C"
{
	void mainSort( const TestInfo *ti, int *data, long size )
	{
		lbkmergesort( ti, data );
	}

	void sort( const TestInfo *ti )
	{
		lbkmergesort( ti, 0 );
	}
}
