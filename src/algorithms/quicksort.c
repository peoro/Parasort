
/**
 * @file quicksort.c
 *
 * @brief This file contains a parallel version of quicksort, a standard algorithm to sort atomic elements (integers)
 *
 * To scatter data:
 *   each node will call \scatter, which takes an array \array of \size=M elements.
 *   when \scatter returns, only the first \size items of \array will be valid:
 *     \array will contain the partition each node must sort.
 * To gather back data:
 *   each node, but node 0, sends its sorted partition to node 0.
 *   node 0 will sequentially read data from any node, into the original array,
 *     (at the right offset) so that no merging will be required.
 *
 *
 * @author Paolo Giangrandi
 * @version 0.0.01
 */

#include "sorting.h"
#include "common.h"
#include "string.h"
#include "dal_internals.h"

#include <time.h> // srand( time(0) )
#include <assert.h>

/**************************************/
/*               STENCIL              */
/**************************************/

#define ACTIVE_PROCS(ti,step) _pow2(step)
#define GET_STEP_COUNT(ti) _log2( GET_N(ti) )

//from_who	: return the rank of the process (node) from which i RECEIVE data in the current step
int from_who( const TestInfo *ti, int step )
{
	switch( ti->algoVar[0] ) {
		case 1: return GET_ID(ti) - ACTIVE_PROCS(ti,step);
	}
	return GET_ID(ti) - ( GET_N(ti) / ACTIVE_PROCS(ti,step+1) );
}

//to_who	: return the rank of the process (node) from which i SEND data in the current step
int to_who( const TestInfo *ti, int step )
{
	switch( ti->algoVar[0] ) {
		case 1: return GET_ID(ti) + ACTIVE_PROCS(ti,step);
	}
	return GET_ID(ti) + ( GET_N(ti) / ACTIVE_PROCS(ti,step+1) );
}

//do_i_send	: true if the calling process (node) has to SEND data TO another process (node) in the current step
bool do_i_send( const TestInfo *ti, int step )
{
	switch( ti->algoVar[0] ) {
		case 1: return GET_ID(ti) < ACTIVE_PROCS(ti,step);
	}
	return ! ( GET_ID(ti) % ( GET_N(ti) / ACTIVE_PROCS(ti,step) ) );
}

//do_i_receive 	: return a number different from 0 if the calling process (node) has to RECEIVE data FROM another process (node) in the current step.
bool do_i_receive( const TestInfo *ti, int step )
{
	switch( ti->algoVar[0] ) {
		case 1: return ! do_i_send( ti, step ) && GET_ID(ti) < 2*ACTIVE_PROCS(ti,step);
	}
	return ! do_i_send( ti, step ) && ! ( GET_ID(ti) % ( GET_N(ti) / ACTIVE_PROCS(ti,step+1) ) ) ;
}

// return the ID of the node owning the n-th data token. Useful during gathering
int nth_token_owner( const TestInfo *ti, int n )
{
	switch( ti->algoVar[0] ) {
		case 1:
		{
			int res = 0;
			int nodes = GET_N(ti);
			int steps = GET_STEP_COUNT(ti);
			int i;

			for( i = 0; i < steps; ++ i ) {
				if( n >= nodes/2 ) {
					res += ACTIVE_PROCS(ti,i);
					n -= nodes/2;
				}
				nodes /= 2;
			}

			return res;
		}
	}
	return n;
}



/**************************************/
/*            ALGORITHMIC             */
/**************************************/

#define SWAP( a, i, j ) { int tmp; tmp = a[i]; a[i] = a[j]; a[j] = tmp; }

#define MIN(a,b) ( (a)<(b) ? (a) : (b) )

/*
// partitions the array:
// a is the array, of size size. p is the pivot
long _partition( int *a, long size, int p )
{
	long i = 0, j = size-1;

	while( true ) {
		while( i < size && a[i] <= p ) { ++ i; }
		while( j > 0 && a[j] > p ) { -- j; }
		if( i >= j || ( i >= size && j < 0 ) ) {
			break;
		}
		SWAP( a, i, j );
	}

	return j;
}
*/

static PhaseHandle computationP; // time for local computation, but communication

// \smaller contain the first partition (the one with smaller numbers than pivot)
// while \bigger will contain the second one (with bigger numbers)
void partition( Data *data, Data *smaller, Data *bigger )
{
	SPD_ASSERT( DAL_isInitialized( smaller ), "smaller Data should be initialized" );
	SPD_ASSERT( DAL_isInitialized( bigger ), "bigger Data should be initialized" );
	SPD_ASSERT( ! DAL_isInitialized( data ), "data to be partitioned should already contain some data!")
	
	switch( data->medium ) {
		case File:
		case Array: {
			int pivot;
			
			// reading pivot
			{
				// using a 1-item buffer to load pivot from data
				Data buf;
				DAL_init( &buf );
				SPD_ASSERT( DAL_allocArray( &buf, 1 ), "not enough memory..." );
			
				int pivotIndex = rand() % DAL_dataSize(data);
				DAL_dataCopyOS( data, pivotIndex, &buf, 0, 1 ); // reading only pivot
				pivot = buf.array.data[ 0 ];
				
				DAL_destroy( &buf );
			}
			
			// allocating partitions
			DAL_allocData( smaller, DAL_dataSize(data) );
			DAL_allocData( bigger, DAL_dataSize(data) );
			dal_size_t smallIdx = 0, bigIdx = 0; // items copied into \smaller and \bigger
			
			// initializing buffers
			Data bufB, bufS, bufD; // buffers for bigger, smaller and data
			dal_size_t bufSize; // size of the three buffers
			dal_size_t bbIdx, bsIdx;
			{
				DAL_init( &bufB );
				DAL_init( &bufS );
				DAL_init( &bufD );
				
				DAL_allocBuffer( &bufB, DAL_dataSize(data) );
				DAL_allocBuffer( &bufS, DAL_dataSize(data) );
				DAL_allocBuffer( &bufD, DAL_dataSize(data) );
				bufSize = MIN( DAL_dataSize(&bufB), MIN( DAL_dataSize(&bufS), DAL_dataSize(&bufD) ) );
				// these should let buffers in memory...
				DAL_reallocData( &bufB, bufSize );
				DAL_reallocData( &bufS, bufSize );
				DAL_reallocData( &bufD, bufSize );
				
				DAL_ASSERT( bufB.medium == Array, &bufB, "Buffer for bigger partition not in memory..." );
				DAL_ASSERT( bufS.medium == Array, &bufS, "Buffer for smaller partition not in memory..." );
				DAL_ASSERT( bufD.medium == Array, &bufD, "Buffer for data not in memory..." );
			}
			
			dal_size_t i, j;
			for( i = 0; i < DAL_dataSize(data); i += bufSize ) {
				DAL_dataCopyOS( data, i, &bufD, 0, bufSize ); // reading one block from \data
				
				// partitioning data in buffers \smaller and \bigger
				bbIdx = bsIdx = 0;
				for( j = 0; j < bufSize; ++ j ) {
					if( bufD.array.data[j] > pivot ) {
						DAL_dataCopyOS( &bufD, j, &bufB, bbIdx, 1 );
						bbIdx ++;
					} else {
						DAL_dataCopyOS( &bufD, j, &bufS, bsIdx, 1 );
						bsIdx ++;
					}
				}
				
				// flushing out \smaller and \bigger
				DAL_dataCopyOS( &bufB, 0, bigger, bigIdx, bbIdx );
				bigIdx += bbIdx;
				DAL_dataCopyOS( &bufS, 0, smaller, smallIdx, bsIdx );
				smallIdx += bsIdx;
			}
			
			DAL_reallocData( smaller, smallIdx );
			DAL_reallocData( bigger, bigIdx );
			
			// destroying buffers
			{
				DAL_destroy( &bufB );
				DAL_destroy( &bufS );
				DAL_destroy( &bufD );
			}
			
#if 0
			int pivotIndex = rand() % data->array.size;
			int pivot = data->array.data[ pivotIndex ];
			long lim = _partition( data->array.data, data->array.size, pivot, pivotIndex );

				//char buf[128];
				//SPD_DEBUG( "partitioned(%d):%s, lim:%ld", pivot, DAL_dataItemsToString(data, buf, sizeof(buf)), lim );
			/*
			// FIXME
			// NOTE: THIS IS NOT SAFE IN GENERAL, SINCE SECOND PARTITION'S DATA WILL GET
			//       DESTROIED WHEN FIRST PARTITION IS DESTROYED!!!
			//       BESIDES DATA2 CANNOT BE DESTROYED!!!

			// putting second partition in a Data
			Data r;
			DAL_init( &r );
			r.medium = Array;
			r.array.data = data->array.data + lim;
			r.array.size = data->array.size - lim;

			// shrinking first partition
			data->array.size = lim;
			*/

			SPD_ASSERT( DAL_allocArray( bigger, data->array.size - lim ), "not enough memory..." );
			memcpy( bigger->array.data, data->array.data + lim, (data->array.size-lim)*sizeof(int) );

			SPD_ASSERT( DAL_reallocArray( data, lim ), "wtf, I'm not even growing it here..." );
			
			// moving data from smaller to data
			DAL_dataSwap( data, smaller );
#endif
			break;
		}
		default:
			DAL_UNSUPPORTED( data );
	}
}

void scatter( const TestInfo *ti, Data *data )
{
	int step = 0;

	for( step = 0; step < GET_STEP_COUNT(ti); ++ step ) {
		if( do_i_send(ti,step) ) {
			//SPD_DEBUG( "partitioning..." );
			Data smaller, bigger;
			DAL_init( &smaller );
			DAL_init( &bigger );
			resumePhase( ti, computationP );
				partition( data, &smaller, &bigger );
			stopPhase( ti, computationP );
			DAL_destroy( data );

				//char buf1[128], buf2[128];
				//SPD_DEBUG( "partition1:%s, partition2:%s", DAL_dataItemsToString(&smaller, buf1, sizeof(buf1)), DAL_dataItemsToString(&bigger, buf2, sizeof(buf2)) );

			//SPD_DEBUG( "need to send data to %d", to_who(ti, step) );
			DAL_sendU( &bigger, to_who(ti, step) );
			//SPD_DEBUG( "data sent" );
			DAL_dataSwap( data, &smaller ); // putting my partition back into \data
			DAL_destroy( &bigger );
		}
		if( do_i_receive(ti,step) ) {
			//SPD_DEBUG( "need to receive data from %d", from_who(ti, step) );
			DAL_receiveU( data, from_who(ti, step) );
			//SPD_DEBUG( "data received" );
		}
	}
	//SPD_DEBUG( "finished scattered!" );

	// printf( "Node %d has got %ld data\n", GET_ID(ti), *size );
}

void gather( const TestInfo *ti, Data *data )
{
	// node 0
	if( ! GET_ID(ti) ) {
		/*
		int actualSize = *size;
		int i;
		// receiving sequentially from ohter nodes
		for( i = 1; i < GET_N(ti); ++ i ) {
			MPI_Status stat;
			_MPI_Recv( a+actualSize, GET_M(ti)-actualSize, MPI_INT, nth_token_owner(ti,i), 0, MPI_COMM_WORLD, &stat );
			_MPI_Get_count( &stat, MPI_INT, size );
			actualSize += *size;
		}
		*size = actualSize;
		*/

		int i;
		// receiving sequentially from ohter nodes
		for( i = 1; i < GET_N(ti); ++ i ) {
			DAL_receiveAU( data, nth_token_owner(ti,i) );
		}
	}
	// other nodes
	else {
		// _MPI_Send ( a, *size, MPI_INT, 0, 0, MPI_COMM_WORLD );
		// *size = 0;
		DAL_sendU( data, 0 );
		DAL_destroy( data );
	}
}


/**************************************/
/*              SORTING               */
/**************************************/

void actualSort( const TestInfo *ti, Data *data )
{
	PhaseHandle sortingP, sequentialSortP;
	srand( time(0) );
	
	// init phase "computation
	computationP = startPhase( ti, "computation" );
	stopPhase( ti, computationP );

	sortingP = startPhase( ti, "sorting" );
		scatter( ti, data ); // scattering

		resumePhase( ti, computationP );
			sequentialSortP = startPhase( ti, "sequential sort" );
			sequentialSort( ti, data );
			stopPhase( ti, sequentialSortP );
		stopPhase( ti, computationP );
	stopPhase( ti, sortingP );

	gather( ti, data ); // gathering
}

void sort( const TestInfo *ti )
{
	Data data;
	DAL_init( &data );

	actualSort( ti, &data );

	DAL_ASSERT( DAL_isInitialized(&data), &data, "data should have been destroied :F" );
}

void mainSort( const TestInfo *ti, Data *data )
{
	//char buf[128];
	//SPD_DEBUG( "data to sort: %s", DAL_dataItemsToString(data, buf, sizeof(buf)) );
	
	actualSort( ti, data );
	
	//SPD_DEBUG( "sorted data: %s", DAL_dataToString(data, buf, sizeof(buf)) );

	DAL_ASSERT( DAL_dataSize(data) == GET_M(ti), data, "data isn't as big as it was originally..." );
}

