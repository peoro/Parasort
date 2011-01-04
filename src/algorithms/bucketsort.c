
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
#include <mpi.h>
#include "../sorting.h"
#include "../utils.h"

/**
* @brief Gets the number of element to be inserted in each small bucket
*
* @param[in] data       	The data object containing data to be distributed
* @param[in] n     			The number of buckets
* @param[out] lengths    	The array that will contain the small bucket lengths
*/
void getSendCounts( Data *data, const int n, long *lengths )
{
	/* TODO: Implement it the right way!! */

	int i, j;
	const double 	range = INT_MAX / n;				//Range of elements in each bucket

		/* Computing the number of integers to be sent to each process */
	for ( i=0; i<data->array.size; i++ ) {
		j = ((double) data->array.data[i]) / range;
		lengths[j]++;
	}
}

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


	long 			sendCounts[n], recvCounts[n];		//Number of elements in send/receive buffers
	long			sdispls[n], rdispls[n];				//Send/receive buffer displacements
	long			i, j, k;

	PhaseHandle 	scatterP, localP, bucketsP, gatherP;

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
/**************************************** Buckets Construction Phase *******************************************/
/***************************************************************************************************************/

	bucketsP = startPhase( ti, "buckets construction" );

	/* Initializing the sendCounts array */
	memset( sendCounts, 0, n*sizeof(long) );

	/* Computing the number of integers to be sent to each process */
	getSendCounts( data, n, sendCounts );

	/* Informing all processes on the number of elements that will receive */
	MPI_Alltoall( sendCounts, 1, MPI_LONG, recvCounts, 1, MPI_LONG, MPI_COMM_WORLD );

	/* Computing the displacements */
	for ( j=0, k=0, i=0; i<n; i++ ) {
		/* Computing the displacements relative to localData from which to take the outgoing data destined for each process */
		sdispls[i] = k;
		k += sendCounts[i];

		/* Computing the displacements relative to localBucket at which to place the incoming data from each process  */
		rdispls[i] = j;
		j += recvCounts[i];
	}
	/* Sending data to the appropriate processes */
	DAL_alltoallv( data, sendCounts, sdispls, recvCounts, rdispls );

	/* Sorting local bucket */
	sequentialSort( ti, data );

	stopPhase( ti, bucketsP );
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/********************************************** Ghater Phase ***************************************************/
/***************************************************************************************************************/
	gatherP = startPhase( ti, "gathering" );

	/* Gathering the lengths of the all buckets */
	MPI_Gather( &j, 1, MPI_LONG, recvCounts, 1, MPI_LONG, root, MPI_COMM_WORLD );

	/* Computing displacements relative to the output array at which to place the incoming data from each process  */
	if ( id == root ) {
		for ( k=0, i=0; i<n; i++ ) {
			rdispls[i] = k;
			k += recvCounts[i];
		}
	}
	/* Gathering sorted data */
	DAL_gatherv( data, recvCounts, rdispls, root );

	stopPhase( ti, gatherP );
/*--------------------------------------------------------------------------------------------------------------*/
}

void mainSort( const TestInfo *ti, Data *data )
{
	bucketSort( ti, data );
}

void sort( const TestInfo *ti )
{
	Data data;
	DAL_init( &data );
	bucketSort( ti, &data );
	DAL_destroy( &data );
}
