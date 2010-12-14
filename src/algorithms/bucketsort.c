
/**
* @file bucketsort.c
*
* @brief This file contains a set of functions used to sort atomic elements (integers) by using a parallel version of the Bucket Sort
*
* @author Nicola Desogus
* @version 0.0.01
*/

#include <limits.h>
#include <string.h>
#include "../sorting.h"
#include "../utils.h"

/**
* @brief Sorts input data by using a parallel version of Bucket Sort
*
* @param[in] ti        The TestInfo Structure
* @param[in] data      Data to be sorted
*/
void bucketSort( const TestInfo *ti, int *data )
{
	const int		root = 0;                           //Rank (ID) of the root process
	const int		id = GET_ID( ti );                  //Rank (ID) of the process
	const int		n = GET_N( ti );                    //Number of processes
	const long		M = GET_M( ti );                    //Number of data elements
	const long		local_M = GET_LOCAL_M( ti );        //Number of elements assigned to each process
	const long		maxLocal_M = M / n + (0 < M%n);     //Max number of elements assigned to a process
	int				*localData = 0;            			//Local section of the input data

	const double 	range = INT_MAX / n;				//Range of elements in each bucket
	int				*localBucket = 0;					//Local bucket
	long 			bucketLength = 0;

	int 			sendCounts[n], recvCounts[n];		//Number of elements in send/receive buffers
	int				sdispls[n], rdispls[n];				//Send/receive buffer displacements
	int				i, j, k;

	PhaseHandle 	scatterP, localP, bucketsP, gatherP;

	/* Allocating memory */
	localData = (int*) malloc( local_M * sizeof(int) );
	localBucket = (int*) malloc( M * sizeof(int) );

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
	qsort( localData, local_M, sizeof(int), compare );

	stopPhase( ti, localP );
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/**************************************** Buckets Construction Phase *******************************************/
/***************************************************************************************************************/

	bucketsP = startPhase( ti, "buckets construction" );

	/* Initializing the sendCounts array */
	memset( sendCounts, 0, n*sizeof(int) );

	/* Computing the number of integers to be sent to each process */
	for ( i=0; i<local_M; i++ ) {
		j = ((double) localData[i]) / range;
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

	stopPhase( ti, bucketsP );
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/********************************************** Ghater Phase ***************************************************/
/***************************************************************************************************************/
	gatherP = startPhase( ti, "gathering" );

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

	stopPhase( ti, gatherP );
/*--------------------------------------------------------------------------------------------------------------*/

	/* Freeing memory */
	free( localData );
	free( localBucket );
}

void mainSort( const TestInfo *ti, int *data, long size )
{
	bucketSort( ti, data );
}

void sort( const TestInfo *ti )
{
	bucketSort( ti, 0 );
}
