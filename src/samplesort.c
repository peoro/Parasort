
/**
 * @file samplesort.c
 *
 * @brief This file contains a set of functions used to sort atomic elements (integers) by using a parallel version of the Sample Sort
 *
 * NOTE: compile with
 *		 export ALGO=samplesort && mpicc -fPIC -O2 -shared -Wl,-soname,lib$ALGO.so -o lib$ALGO.so $ALGO.c && cp lib$ALGO.so ~/.spd/algorithms/
 *
 * @author Nicola Desogus
 * @version 0.0.01
 */



#include "sorting.h"


/**
* @brief Compares the input integers
*
* @param[in] a	The firt element to be compared
* @param[in] b	The second element to be compared
*
* @return A negative value if the first argument is less
		  than the second, zero if they are equal, and
		  positive if the first argument is greater than
		  the second
*/
int intCompare (const void* a, const void* b) {
	return ( *(int*)a - *(int*)b );
}



/**
* @brief Gets the index of the bucket in which to insert ele
*
* @param[in] ele		The element to be inserted within a bucket
* @param[in] splitters	The array of splitters
* @param[in] size		The number of splitters
*
* @return The index of the bucket in which to insert ele
*/
int getBucketIndex( const int* ele, int* splitters, int size ) {
	register int base = 0;
	register int limit = size;
	register int cmpResult;
	register int mid;

	while( limit > base ) {
		mid = (base + limit) >> 1;
		cmpResult = intCompare( ele, &splitters[mid] );

		if( cmpResult == 0 )
			return mid;

		if( cmpResult > 0 )
			base = mid + 1;
		else
			limit >>= 1 ;
	}
	return limit;
}


/**
 * @brief Sorts input data by using a parallel version of the Sample Sort
 *
 * @param[in] ti 		The TestInfo Structure
 * @param[in] data	 	Data to be sorted
 * @param[in] size 		Size (in bytes) of data
 */
void sort( const TestInfo *ti, int *data, long size )
{
	const int		root = 0;							//Rank (ID) of the root process
	const int 		id = GET_ID( ti );					//Rank (ID) of the process
	const int 		n = GET_N( ti );					//Number of processes
// 	const long int 	M = GET_M( ti );					//Number of data elements
	const long int 	local_M = GET_LOCAL_M( ti );		//Number of elements assigned to each process
	int				local_splitters[n-1];				//Local splitters (n-1 equidistant elements of the data array)
	int* 			all_splitters = 0;					//All splitters (will include all the local splitters)
	int* 			global_splitters = 0;				//Global splitters (will be selected from the all_splitters array)
	int				small_buckets[n][2*local_M/n+1];	//n small buckets (in sample sort, it is shown that each bucket will have a maximum of 2*local_M/n elements)
	int 			received_buckets[n][2*local_M/n+1];	//n received buckets (in sample sort, it is shown that each bucket will have a maximum of 2*local_M/n elements)
	int				local_bucket[2*local_M];			//Local bucket (in sample sort, it is shown that each process will have, at the end, a maximum of 2*M/n elements to sort)
	int 			i, j, k;


	/* Sorting locally */
	qsort( data, local_M, sizeof(int), intCompare );

	/* Choosing local splitters (n-1 equidistant elements of the data array) */
	j = local_M / n;
	k = j;
	for( i=0; i<n-1; i++, k+=j )
		local_splitters[i] = data[k];

	/* Gathering all splitters to the root process */
	if( id == root ) {
		all_splitters = (int*) malloc ( ((n-1)*n) * sizeof(int) );

		if( all_splitters == NULL ) {
			printf( "Error: Cannot allocate memory for all_splitters array! \n");
			MPI_Abort( MPI_COMM_WORLD, MPI_ERR_OTHER );
			return;
		}
	}
	MPI_Gather( local_splitters, n-1, MPI_INT, all_splitters, n-1, MPI_INT, root, MPI_COMM_WORLD );

	/* Choosing global splitters (n-1 equidistant elements of the all_splitters array) */
	global_splitters = local_splitters; 	//--> To save space but keeping the correct semantics!!! :F

	if( id == root ) {
		qsort( all_splitters, (n-1)*n, sizeof(int), intCompare );		//Sorts the all_splitters array

		j = n-1;
		k = j;
		for( i=0; i<n-1; i++, k+=j )
			global_splitters[i] = all_splitters[k];
	}

	/* Broadcasting global splitters */
	MPI_Bcast( global_splitters, n-1, MPI_INT, root, MPI_COMM_WORLD );

	for( i=0; i<n; i++ ) {
		small_buckets[i][0] = 1;
	}

	/* Splitting local data into n small buckets */
	for( i=0; i<local_M; i++ ) {
		/* Placing the element in the right bubket */
		j = getBucketIndex( &data[i], global_splitters, n-1 );
		small_buckets[j][small_buckets[j][0]++] = data[i];
	}

	/* Sending small buckets to respective processes */
	k = 2*local_M/n+1;	//Number of elements to be sent to each process
	MPI_Alltoall( small_buckets, k, MPI_INT, received_buckets, k, MPI_INT, MPI_COMM_WORLD );

	/* Retrieving local bucket elements */
	k = 0;

	for( i=0; i<n; i++ ) {
		for( j=1; j<received_buckets[i][0]; j++ )
			local_bucket[k++] = received_buckets[i][j];
	}

	/* Sorting local bucket */
	qsort( local_bucket, k, sizeof(int), intCompare );

	//TODO: Store data correctly -> local_bucket is sorted, but could be greater than initial data (> M/n)

	/* Freeing memory */
	if( id == root )
		free( all_splitters );
}
