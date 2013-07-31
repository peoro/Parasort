
#include <mpi.h>
#include <sys/time.h>
#include "../debug.h"

#define TESTS_ERROR( ret, fmt, ... ) \
	{ \
		SPD_DEBUG( fmt, ##__VA_ARGS__ ); \
		return ret; \
	}

// tiny wrappers for MPI
// these don't use Datas, memory arrays like MPI do, but are so much more comfortable for our purpose...
// taken from dal.c
static inline void TESTS_MPI_SEND( void *array, long size, MPI_Datatype dataType, int dest ) {
	MPI_Send( array, size, dataType, dest, 0, MPI_COMM_WORLD );
}
static inline void TESTS_MPI_RECEIVE( void *array, long size, MPI_Datatype dataType, int source ) {
	MPI_Status 	stat;
	MPI_Recv( array, size, dataType, source, 0, MPI_COMM_WORLD, &stat );
}

static inline void TESTS_MPI_SCATTER ( void *send_array, long size_per_proc, MPI_Datatype dataType, void *recv_array, int root ) {
	MPI_Scatter ( send_array, size_per_proc, dataType, recv_array, size_per_proc, dataType, root, MPI_COMM_WORLD ); 
}

static inline void TESTS_MPI_GATHER ( void *send_array, long size, long size_per_proc, MPI_Datatype dataType, void *recv_array, int root ) {
	MPI_Gather ( send_array, size_per_proc, dataType, recv_array, size_per_proc, dataType, root, MPI_COMM_WORLD ); 
}

static inline void TESTS_MPI_ALLTOALL ( void *send_array, long size_per_proc, MPI_Datatype dataType, void *recv_array ) {
	MPI_Alltoall ( send_array, size_per_proc, dataType, recv_array, size_per_proc, dataType, MPI_COMM_WORLD ); 
}

static inline int GET_ID ( ) {
    int x;
    MPI_Comm_rank ( MPI_COMM_WORLD, &x );
    return x;
}
static inline int GET_N ( ) {
    int x;
    MPI_Comm_size ( MPI_COMM_WORLD, &x );
    return x;
}

// time functions
struct TimeDiff {
	long utime, mtime, time;
};

typedef struct timeval Time;
typedef struct TimeDiff TimeDiff;

static TimeDiff timeDiff( struct timeval startTime, struct timeval endTime )
{
	long secs, usecs;
	long utime, mtime, time;

	secs  = endTime.tv_sec  - startTime.tv_sec;
	usecs = endTime.tv_usec - startTime.tv_usec;

	utime = secs*1000000 + usecs;
	mtime = (secs*1000 + usecs/1000.0) + 0.5;
	time = secs + usecs/1000000.0 + 0.5; // secs
	
	TimeDiff r = { utime, mtime, time };
	return r;
}
static Time now( )
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	return tv;
}

