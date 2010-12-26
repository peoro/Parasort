
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
void bucketSort( const TestInfo *ti, Data *data )
{
	const int		root = 0;                           //Rank (ID) of the root process
	const int		id = GET_ID( ti );                  //Rank (ID) of the process
	const int		n = GET_N( ti );                    //Number of processes
	const long		M = GET_M( ti );                    //Number of data elements
	const long		local_M = GET_LOCAL_M( ti );        //Number of elements assigned to each process
	const long		maxLocal_M = M / n + (0 < M%n);     //Max number of elements assigned to a process

	const double 	range = INT_MAX / n;				//Range of elements in each bucket

	Data			localBucket; 						//Local bucket
	long 			bucketLength = 0;

	long 			sendCounts[n], recvCounts[n];		//Number of elements in send/receive buffers
	long			sdispls[n], rdispls[n];				//Send/receive buffer displacements
	long			i, j, k;

	PhaseHandle 	scatterP, localP, bucketsP, gatherP;

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
	scatterv( ti, data, sendCounts, sdispls, root );

	stopPhase( ti, scatterP );
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/*********************************************** Local Phase ***************************************************/
/***************************************************************************************************************/

	localP = startPhase( ti, "local sorting" );

/*TODO*/	data->size = local_M;

	/* Sorting local data */
	sequentialSort( ti, data );

/*TODO*/	data->size = M;

	stopPhase( ti, localP );
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/**************************************** Buckets Construction Phase *******************************************/
/***************************************************************************************************************/

	bucketsP = startPhase( ti, "buckets construction" );

	/* Initializing the sendCounts array */
	memset( sendCounts, 0, n*sizeof(long) );

	/* Computing the number of integers to be sent to each process */
/*TODO*/	for ( i=0; i<local_M; i++ ) {
/*TODO*/		j = ((double) data->array[i]) / range;
/*TODO*/		sendCounts[j]++;
/*TODO*/	}
	/* Informing all processes on the number of elements that will receive */
	MPI_Alltoall( sendCounts, 1, MPI_LONG, recvCounts, 1, MPI_LONG, MPI_COMM_WORLD );

	/* Computing the displacements */
	for ( k=0, i=0; i<n; i++ ) {
		/* Computing the displacements relative to localData from which to take the outgoing data destined for each process */
		sdispls[i] = k;
		k += sendCounts[i];

		/* Computing the displacements relative to localBucket at which to place the incoming data from each process  */
		rdispls[i] = bucketLength;
		bucketLength += recvCounts[i];
	}
/*TODO*/	allocDataArray( &localBucket, bucketLength );
	/* Sending data to the appropriate processes */
	alltoallv( ti, data, sendCounts, sdispls, &localBucket, recvCounts, rdispls );

	/* Sorting local bucket */
	sequentialSort( ti, &localBucket );

	stopPhase( ti, bucketsP );
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/********************************************** Ghater Phase ***************************************************/
/***************************************************************************************************************/
	gatherP = startPhase( ti, "gathering" );

	/* Gathering the lengths of the all buckets */
	MPI_Gather( &bucketLength, 1, MPI_LONG, recvCounts, 1, MPI_LONG, root, MPI_COMM_WORLD );

	/* Computing displacements relative to the output array at which to place the incoming data from each process  */
	if ( id == root ) {
		for ( k=0, i=0; i<n; i++ ) {
			rdispls[i] = k;
			k += recvCounts[i];
		}
		/*TODO*/	memcpy( data->array, localBucket.array, bucketLength*sizeof(int) );
	}
	else {
		/*TODO: Swap!*/
/*TODO*/	int *tmp;
/*TODO*/	tmp = data->array;
/*TODO*/	data->array = localBucket.array;
/*TODO*/	data->size = bucketLength;
/*TODO*/	localBucket.array = tmp;
/*TODO*/	localBucket.size = data->size;
	}

	/* Gathering sorted data */
	gatherv( ti, data, recvCounts, rdispls, root );

	stopPhase( ti, gatherP );
/*--------------------------------------------------------------------------------------------------------------*/

	/* Freeing memory */
	if ( id != root )
		destroyData( data );
	destroyData( &localBucket );
}

void mainSort( const TestInfo *ti, Data *data )
{
	bucketSort( ti, data );
}

void sort( const TestInfo *ti )
{
	Data data;
	bucketSort( ti, &data );
}
