
/**
* @file samplesort.c
*
* @brief This file contains a set of functions used to sort atomic elements (integers) by using a parallel version of the Sample Sort
*
* @author Nicola Desogus
* @version 0.0.01
*/

#include "../sorting.h"
#include "../utils.h"

/**
* @brief Gets the index of the bucket in which to insert ele
*
* @param[in] ele        The element to be inserted within a bucket
* @param[in] splitters  The array of splitters
* @param[in] length     The number of splitters
*
* @return The index of the bucket in which to insert ele
*/
int getBucketIndex( const int *ele, const int *splitters, int length )
{
	int base = 0;
	int cmpResult;
	int mid;

	while ( length > base ) {
		mid = (base + length) >> 1;
		cmpResult = compare( ele, &splitters[mid] );

		if ( cmpResult == 0 )
			return mid;

		if ( cmpResult > 0 )
			base = mid + 1;
		else
			length >>= 1;
	}
	return base;
}

/**
* @brief Chooses n-1 equidistant elements of the input data array as splitters
*
* @param[in] 	data 				Data from which extract splitters
* @param[in] 	length 				Length of the data array
* @param[in] 	n 					Number of splitters plus one
* @param[out] 	newSplitters	 	Chosen splitters
*/
void chooseSplitters( int *data, const int length, const int n, int *newSplitters )
{
	int i, j, k;

	/* Sorting data */
	qsort( data, length, sizeof(int), compare );

	/* Choosing splitters (n-1 equidistant elements of the data array) */
	for ( i=0, k=j=length/n; i<n-1; i++, k+=j )
		newSplitters[i] = data[k];
}

/**
* @brief Sorts input data by using a parallel version of Sample Sort
*
* @param[in] ti        The TestInfo Structure
* @param[in] data      Data to be sorted
*/
void sampleSort( const TestInfo *ti, int *data )
{
	const int	root = 0;                           //Rank (ID) of the root process
	const int	id = GET_ID( ti );                  //Rank (ID) of the process
	const int	n = GET_N( ti );                    //Number of processes
	const long	M = GET_M( ti );                    //Number of data elements
	const long	local_M = GET_LOCAL_M( ti );        //Number of elements assigned to each process
	const long	maxLocal_M = M / n + (0 < M%n);     //Max number of elements assigned to a process
	int			localData[local_M];                	//Local section of the input data

	int			localSplitters[n-1];               	//Local splitters (n-1 equidistant elements of the data array)
	int			*allSplitters = 0;                 	//All splitters (will include all the local splitters)
	int			*globalSplitters = 0;            	//Global splitters (will be selected from the allSplitters array)

	int			localBucket[2*maxLocal_M]; 			//Local bucket (in sample sort, it is shown that each process will have, at the end, a maximum of 2*M/n elements to sort)
	int 		bucketLength = 0;

	int 		sendCounts[n];
	int			sdispls[n];

	int 		recvCounts[n];
	int			rdispls[n];

	int			i, j, k;

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

	/* Choosing local splitters (n-1 equidistant elements of the data array) */
	chooseSplitters( localData, local_M, n, localSplitters );

	/* Gathering all splitters to the root process */
	if ( id == root ) {
		allSplitters = (int*) malloc ( ((n-1)*n) * sizeof(int) );

		if ( allSplitters == NULL ) {
			printf( "Error: Cannot allocate memory for allSplitters array! \n");
			MPI_Abort( MPI_COMM_WORLD, MPI_ERR_OTHER );
			return;
		}
	}
	MPI_Gather( localSplitters, n-1, MPI_INT, allSplitters, n-1, MPI_INT, root, MPI_COMM_WORLD );

	/* Choosing global splitters (n-1 equidistant elements of the allSplitters array) */
	globalSplitters = localSplitters;     //--> To save space but keeping the correct semantics!!! :F

	if ( id == root )
		chooseSplitters( allSplitters, (n-1)*n, n, globalSplitters );

	/* Broadcasting global splitters */
	MPI_Bcast( globalSplitters, n-1, MPI_INT, root, MPI_COMM_WORLD );

	/* Initializing the sendCounts array */
	for ( i=0; i<n; i++ ) {
		sendCounts[i] = 0;
	}

	/* Computing the number of integers to be sent to each process */
	for ( i=0; i<local_M; i++ ) {
		j = getBucketIndex( &localData[i], globalSplitters, n-1 );
		sendCounts[j]++;
	}
	/* Informing all processes on the number of elements that will receive */
	MPI_Alltoall( sendCounts, 1, MPI_INT, recvCounts, 1, MPI_INT, MPI_COMM_WORLD );

	/* Computing the displacements */
	for ( k=0, i=0; i<n; i++ ) {
		/* Computing the displacements relative to localData from which to take the outgoing data destined for each process */
		sdispls[i] = k;
		k += sendCounts[i];

		/* Computing the displacements relative to localBucket at which to place the incoming data from each process  */
		rdispls[i] = bucketLength;
		bucketLength += recvCounts[i];
	}
	/* Sending data to the appropriate processes */
	MPI_Alltoallv( localData, sendCounts, sdispls, MPI_INT, localBucket, recvCounts, rdispls, MPI_INT, MPI_COMM_WORLD );

	/* Sorting local bucket */
	qsort( localBucket, bucketLength, sizeof(int), compare );

	/* Gathering the lengths of the all buckets */
	MPI_Gather( &bucketLength, 1, MPI_INT, recvCounts, 1, MPI_INT, root, MPI_COMM_WORLD );

	/* Computing displacements relative to the output array at which to place the incoming data from each process  */
	if ( id == root ) {
		for ( k=0, i=0; i<n; i++ ) {
			rdispls[i] = k;
			k += recvCounts[i];
		}
	}
	/* Gathering sorted data */
	MPI_Gatherv( localBucket, bucketLength, MPI_INT, data, recvCounts, rdispls, MPI_INT, root, MPI_COMM_WORLD );

	/* Freeing memory */
	if ( id == root )
		free( allSplitters );
}

void mainSort( const TestInfo *ti, int *data, long size )
{
	sampleSort( ti, data );
}

void sort( const TestInfo *ti )
{
	sampleSort( ti, 0 );
}
