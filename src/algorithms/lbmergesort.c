
/**
* @file lbmergesort.c
*
* @brief This file contains a set of functions used to sort atomic elements (integers) by using a parallel version of Mergesort with load balancing
*
* @author Nicola Desogus
* @version 0.0.01
*/

#include "../sorting.h"
#include "../utils.h"
#include <string.h>

/**
* @brief Merges two sorted sequences
*
* @param[in] 	flength			The length of the first sequence
* @param[in] 	firstSeq		The first sequence
* @param[in] 	slength			The length of the second sequence
* @param[in] 	secondSeq    	The second sequence
* @param[out] 	mergedSeq		The merged sequence
*/
void merge( const int flength, int *firstSeq, const int slength, int *secondSeq, int* mergedSeq )
{
	int i, j, k;

	i = 0; j = 0; k = 0;

	while ( j < flength && k < slength )
		if ( firstSeq[j] < secondSeq[k] )
			mergedSeq[i++] = firstSeq[j++];
		else
			mergedSeq[i++] = secondSeq[k++];

	for ( ; j<flength; j++ )
		mergedSeq[i++] = firstSeq[j];

	for ( ; k<slength; k++ )
		mergedSeq[i++] = secondSeq[k];
}



/**
* @brief Sorts input data by using a parallel version of Mergesort with load balancing
*
* @param[in] ti        The TestInfo Structure
* @param[in] data      Data to be sorted
*/
void lbmergeSort( const TestInfo *ti, int *data )
{
	const int	root = 0;                           	//Rank (ID) of the root process
	const int	id = GET_ID( ti );                  	//Rank (ID) of the process
	const int	n = GET_N( ti );                    	//Number of processes
	const long	M = GET_M( ti );                    	//Number of data elements
	const long	maxLocal_M = M / n + (0 < M%n);     	//Max number of elements assigned to a process

	int			*localData, *recvData, *mergedData;
	long 		dataLength = GET_LOCAL_M( ti );        	//Number of elements assigned to this process

	int 		sendCounts[n], recvCounts[n];			//Number of elements in send/receive buffers
	int			sdispls[n], rdispls[n];					//Send/receive buffer displacements

	MPI_Status 	status;

	int 		groupSize, idInGroup, partner, pairedGroupRoot, groupRoot;

	int			groupSplitters[n*(n-1)], recvSplitters[n*(n-1)], mergedSplitters[n*(n-1)], splitters[n-1];
	int 		splittersCount = 0;

	int			i, j, k, h;

	PhaseHandle scatterP, localP, gatherP;

	/* Allocating memory */
	localData = (int*) malloc( M/2 * sizeof(int) );
	recvData = (int*) malloc( M/2 * sizeof(int) );
	mergedData = (int*) malloc( M/2 * sizeof(int) );

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

	/* Choosing local splitters (n-1 equidistant elements of the data array) */
	chooseSplitters( localData, dataLength, n, groupSplitters );

	for ( i=1, groupSize=1; i<=_log2( n ); i++, groupSize<<=1 ) {
		splittersCount += groupSize;	//Updates the number of splitters needed in this step
		partner = id ^ groupSize;		//Selects as partner the process with rank that differs from id only at the i-th bit
		idInGroup = id % groupSize;		//Retrieves the id of the process within the group

		/* Retrieving the id of the group root and the paired group root */
		groupRoot = id-idInGroup;
		pairedGroupRoot = (id&groupSize) ? groupRoot-groupSize : groupRoot+groupSize;

		/* Exchanging local group splitters with partner */
		MPI_Sendrecv( groupSplitters, groupSize*(n-1), MPI_INT, partner, 100, recvSplitters, groupSize*(n-1), MPI_INT, partner, 100, MPI_COMM_WORLD, &status );

		/* Merging local group splitters with received splitters */
		merge( groupSize*(n-1), groupSplitters, groupSize*(n-1), recvSplitters, mergedSplitters );

		/* Updating group splitters splitters for the next step */
 		memcpy( groupSplitters, mergedSplitters, 2*groupSize*(n-1)*sizeof(int) );

		/* Selecting the splitters for this step */
		chooseSplitters( mergedSplitters, 2*groupSize*(n-1), splittersCount+1, splitters );

		/* Initializing the sendCounts array */
		memset( sendCounts, 0, n*sizeof(int) );

		/* Computing the number of integers to be sent to each process */
		h = id < partner ? groupRoot : pairedGroupRoot;
		for ( j=0; j<dataLength; j++ ) {
			k = getBucketIndex( &localData[j], splitters, splittersCount );
			sendCounts[h+k]++;
		}

		/* Computing the displacements */
		for ( k=0, j=0; j<n; j++ ) {
			/* Computing the displacements relative to localData from which to take the outgoing data destined for each process */
			sdispls[j] = k;
			k += sendCounts[j];
		}

		int recvDataLength = 0;
		int sentData = 0;
		MPI_Request r;

		for ( j=0; j<2*groupSize; j++, h++ )
			if ( h != id ) {
				MPI_Isend( &localData[sdispls[h]], sendCounts[h], MPI_INT, h, 10, MPI_COMM_WORLD, &r );
				MPI_Recv( &recvData[recvDataLength], M/2, MPI_INT, h, 10, MPI_COMM_WORLD, &status );

				sentData += sendCounts[h];

				MPI_Get_count( &status, MPI_INT, &k);
				recvDataLength += k;
			}
		/* Sorting and merging received data */
		qsort( recvData, recvDataLength, sizeof(int), compare );
		merge( recvDataLength, recvData, dataLength-sentData, &localData[sdispls[id]], mergedData );

		dataLength = dataLength - sentData + recvDataLength;

 		memcpy( localData, mergedData, dataLength*sizeof(int) );
	}

	stopPhase( ti, localP );
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
	}
	/* Gathering sorted data */
	MPI_Gatherv( localData, dataLength, MPI_INT, data, recvCounts, rdispls, MPI_INT, root, MPI_COMM_WORLD );

	stopPhase( ti, gatherP );
/*--------------------------------------------------------------------------------------------------------------*/

	/* Freeing memory */
	free( localData );
	free( recvData );
	free( mergedData );
}

void mainSort( const TestInfo *ti, int *data, long size )
{
	lbmergeSort( ti, data );
}

void sort( const TestInfo *ti )
{
	lbmergeSort( ti, 0 );
}
