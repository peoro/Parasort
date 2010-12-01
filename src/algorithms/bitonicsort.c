
/**
* @file bitonicsort.c
*
* @brief This file contains a set of functions used to sort atomic elements (integers) by using a parallel version of the Bitonic Sort
*
* @author Nicola Desogus
* @version 0.0.01
*/

#include <string.h>
#include "../sorting.h"
#include "../utils.h"

/**
* @brief Compare and Exchange operation on the low part of two sequences sorted in ascending order
*
* @param[in] 	length			The length of the sequences
* @param[in] 	firstSeq		The first sequence
* @param[in] 	secondSeq    	The second sequence
* @param[out] 	mergedSeq		The merged sequence
*/
void compareLow( const int length, int *firstSeq, int *secondSeq, int* mergedSeq )
{
	int i, j, k;

	for ( i=j=k=0; i<length; i++ )
		if ( secondSeq[j] <= firstSeq[k] )
			mergedSeq[i] = secondSeq[j++];
		else
			mergedSeq[i] = firstSeq[k++];
}


/**
* @brief Compare and Exchange operation on the high part of two sequences sorted in ascending order
*
* @param[in] 	length			The length of the sequences
* @param[in] 	firstSeq		The first sequence
* @param[in] 	secondSeq    	The second sequence
* @param[out] 	mergedSeq		The merged sequence
*/
void compareHigh( const int length, int *firstSeq, int *secondSeq, int* mergedSeq )
{
	int i, j, k;

	for ( i=j=k=length-1; i>=0; i-- )
		if ( secondSeq[j] >= firstSeq[k] )
			mergedSeq[i] = secondSeq[j--];
		else
			mergedSeq[i] = firstSeq[k--];
}

/**
* @brief Sorts input data by using a parallel version of Bitonic Sort
*
* @param[in] ti        The TestInfo Structure
* @param[in] data      Data to be sorted
*/
void bitonicSort( const TestInfo *ti, int *data )
{
	const int		root = 0;                           //Rank (ID) of the root process
	const int		id = GET_ID( ti );                  //Rank (ID) of the process
	const int		n = GET_N( ti );                    //Number of processes
	const long		M = GET_M( ti );                    //Number of data elements
	const long		local_M = GET_LOCAL_M( ti );        //Number of elements assigned to each process
	const long		maxLocal_M = M / n + (0 < M%n);     //Max number of elements assigned to a process
	int				*localData = 0;	                	//Local section of the input data

	int				*recvData = 0;
	int				*mergedData = 0;

	MPI_Status 		status;

	int 			counts[n];							//Number of elements in send/receive buffers
	int				displs[n];							//Send/receive buffer displacements

	int				mask, mask2, partner, recvCount, length;
	int				i, j, k, z, flag;

	PhaseHandle 	scatterP, localP, gatherP;

	/* Allocating memory */
	localData = (int*) malloc( local_M * sizeof(int) );
	recvData = (int*) malloc( maxLocal_M * sizeof(int) );
	mergedData = (int*) malloc( maxLocal_M * sizeof(int) );

/***************************************************************************************************************/
/********************************************* Scatter Phase ***************************************************/
/***************************************************************************************************************/
	scatterP = startPhase( ti, "scattering" );

	/* Computing the number of elements to be sent to each process and relative displacements */
	if ( id == root ) {
		for ( k=0, i=0; i<n; i++ ) {
			displs[i] = k;
			counts[i] = M/n + (i < M%n);
			k += counts[i];
		}
	}
	/* Scattering data */
	MPI_Scatterv( data, counts, displs, MPI_INT, localData, maxLocal_M, MPI_INT, root, MPI_COMM_WORLD );

	stopPhase( ti, scatterP );
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/*********************************************** Local Phase ***************************************************/
/***************************************************************************************************************/
	localP = startPhase( ti, "local sorting" );

	/* Sorting local data */
	qsort( localData, local_M, sizeof(int), compare );

	k = _log2( n );		//Number of steps of the outer loop

	/* Bitonic Sort */
	for ( i=1, mask=2; i<=k; i++, mask<<=1 ) {
		mask2 = 1 << (i - 1);					//Bitmask for partner selection
		flag = (id & mask) == 0 ? -1 : 1;		//flag=-1 iff id has a 0 at the i-th bit, flag=1 otherwise (NOTE: mask = 2^i)

		for ( j=0; j<i; j++, mask2>>=1 ) {
			partner = id ^ mask2;				//Selects as partner the process with rank that differs from id only at the j-th bit
			recvCount = M/n + (partner < M%n);	//Number of elements that will be received from partner

			/* Length of the merged sequence */
			length = recvCount < local_M ? recvCount : local_M;

			/* Exchanging data with partner */
			MPI_Sendrecv( localData, local_M, MPI_INT, partner, 100, recvData, recvCount, MPI_INT, partner, 100, MPI_COMM_WORLD, &status );

			/* Each process must call the dual function of its partner */
			if ( (id-partner) * flag > 0 )
				compareLow( length, recvData, localData, mergedData );
			else
				compareHigh( length, recvData, localData, mergedData );

			/* Saving the merged sequence as local data for the next step */
			memcpy( localData, mergedData, length*sizeof(int) );
		}
	}

	stopPhase( ti, localP );
/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/********************************************** Ghater Phase ***************************************************/
/***************************************************************************************************************/
	gatherP = startPhase( ti, "gathering" );

	/* Gathering sorted data */
	MPI_Gatherv( localData, local_M, MPI_INT, data, counts, displs, MPI_INT, root, MPI_COMM_WORLD );

	stopPhase( ti, gatherP );
/*--------------------------------------------------------------------------------------------------------------*/

	/* Freeing memory */
	free( localData );
	free( recvData );
	free( mergedData );
}

void mainSort( const TestInfo *ti, int *data, long size )
{
	bitonicSort( ti, data );
}

void sort( const TestInfo *ti )
{
	bitonicSort( ti, 0 );
}
