
/**
* @file bucketsort.c
*
* @brief This file contains a set of functions used to sort atomic elements (integers) by using a parallel version of the Bitonic Sort
*
* @author Nicola Desogus
* @version 0.0.01
*/

#include "../sorting.h"
#include "../utils.h"

/**
* @brief Compare and Exchange operation on two bitonic sequences starting from the bottom
*
* @param[in] tmpSeq			An auxiliar array
* @param[in] firstLength    The length of the first sequence
* @param[in] firstSeq		The first sequence
* @param[in] secondLength	The length of the second sequence
* @param[out] secondSeq    	The second sequence (it will be also the merged sequence as output)
*/
void CompareLow( int* tmpSeq, const int firstLength, int *firstSeq, const int secondLength, int *secondSeq )
{
	int i, j, k;
	const int length = firstLength < secondLength ? firstLength : secondLength;

	for ( i=j=k=0; i<length; i++ )
		if ( secondSeq[j] <= firstSeq[k] )
			tmpSeq[i] = secondSeq[j++];
		else
			tmpSeq[i] = firstSeq[k++];

	for ( i=0; i<length; i++ )
		secondSeq[i] = tmpSeq[i];
}

/**
* @brief Compare and Exchange operation on two bitonic sequences starting from the top
*
* @param[in] tmpSeq			An auxiliar array
* @param[in] firstLength    The length of the first sequence
* @param[in] firstSeq		The first sequence
* @param[in] secondLength	The length of the second sequence
* @param[out] secondSeq    	The second sequence (it will be also the merged sequence as output)
*/
void CompareHigh( int* tmpSeq, const int firstLength, int *firstSeq, const int secondLength, int *secondSeq )
{
	int i, j, k;
	const int length = firstLength < secondLength ? firstLength : secondLength;

	for ( i=j=k=length-1; i>=0; i-- )
		if ( secondSeq[j] >= firstSeq[k] )
			tmpSeq[i] = secondSeq[j--];
		else
			tmpSeq[i] = firstSeq[k--];

	for ( i=0; i<length; i++ )
		secondSeq[i] = tmpSeq[i];
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
	int				localData[local_M];                	//Local section of the input data

	int				recvData[maxLocal_M];
	int				tmpData[maxLocal_M];

	MPI_Status 		status;

	int 			counts[n];
	int				displs[n];

	int				mask, mask2, partner, recvCount;
	int				i, j, k, z;

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

	/* Sorting local data */
	qsort( localData, local_M, sizeof(int), compare );

	k = _log2( n );		//Number of steps

	/* Bitonic Sort */
	for ( i=1, mask=2; i<=k; i++, mask<<=1 ) {
		mask2 = 1 << (i - 1);					//Bitmask for partner selection
		z = (id & mask) == 0 ? -1 : 1;			//z=-1 iff id is a multiple of mask (=2^i), z=1 otherwise

		for ( j=0; j<i; j++, mask2>>=1 ) {
			partner = id ^ mask2;				//Selects as partner the process with rank that differs from id only at the j-th bit
			recvCount = M/n + (partner < M%n);	//Number of elements that will be received from partner

			/* Each process must call the dual function of its partner */
			if ( (id-partner) * z > 0 ) {
				MPI_Sendrecv( localData, local_M, MPI_INT, partner, 100, recvData, recvCount, MPI_INT, partner, 100, MPI_COMM_WORLD, &status );
				CompareLow( tmpData, recvCount, recvData, local_M, localData );
			}
			else {
				MPI_Sendrecv( localData, local_M, MPI_INT, partner, 100, recvData, recvCount, MPI_INT, partner, 100, MPI_COMM_WORLD, &status );
				CompareHigh( tmpData, recvCount, recvData, local_M, localData );
			}
		}
	}
	/* Gathering sorted data */
	MPI_Gatherv( localData, local_M, MPI_INT, data, counts, displs, MPI_INT, root, MPI_COMM_WORLD );
}

void mainSort( const TestInfo *ti, int *data, long size )
{
	bitonicSort( ti, data );
}

void sort( const TestInfo *ti )
{
	bitonicSort( ti, 0 );
}
