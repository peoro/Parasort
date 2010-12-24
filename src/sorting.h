#ifndef _SORTING_H_
#define _SORTING_H_

#include <mpi.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
extern "C" {
#endif

typedef struct
{
	bool verbose;
	bool threaded; // use multithread between cores

    long M; //data count
    long seed;
    char algo[64];
    int algoVar[3];
} TestInfo;


inline int GET_ID ( const TestInfo *ti ) {
	(void) ti;
    int x;
    MPI_Comm_rank ( MPI_COMM_WORLD, &x );
    return x;
}

inline int GET_N ( const TestInfo *ti ) {
	(void) ti;
    int x;
    MPI_Comm_size ( MPI_COMM_WORLD, &x );
    return x;
}

inline long GET_M ( const TestInfo *ti ) {
    return ti->M;
}

inline long GET_LOCAL_M ( const TestInfo *ti ) {
    long div = GET_M(ti) / GET_N(ti);
    int rem = GET_M(ti) % GET_N(ti);
    return div + ( GET_ID(ti) < rem );
}

inline const char* GET_ALGO ( const TestInfo *ti ) {
    return ti->algo;
}

inline long GET_SEED ( const TestInfo *ti ) {
    return ti->seed;
}

inline char * GET_ALGORITHM_PATH( const char *algo, char *path, int pathLen )
{
	snprintf( path, pathLen, "%s/.spd/algorithms/lib%s.so", getenv("HOME"), algo );
	return path;
}

#ifndef DEBUG
inline char * GET_UNSORTED_DATA_PATH( const TestInfo *ti, char *path, int pathLen )
{
	snprintf( path, pathLen, "%s/.spd/data/M%ld_s%ld.unsorted",
							 getenv("HOME"), GET_M(ti), GET_SEED(ti) );
	return path;
}
inline char * GET_SORTED_DATA_PATH( const TestInfo *ti, char *path, int pathLen )
{
	snprintf( path, pathLen, "%s/.spd/data/M%ld_s%ld.%s.sorted",
							 getenv("HOME"), GET_M(ti), GET_SEED(ti), GET_ALGO(ti) );
	return path;
}
#else
inline char * GET_UNSORTED_DATA_PATH( const TestInfo *ti, char *path, int pathLen )
{
	snprintf( path, pathLen, "%s/.spd/data/M%ld_s%ld_human.unsorted",
							 getenv("HOME"), GET_M(ti), GET_SEED(ti) );
	return path;
}
inline char * GET_SORTED_DATA_PATH( const TestInfo *ti, char *path, int pathLen )
{
	snprintf( path, pathLen, "%s/.spd/data/M%ld_s%ld_human.%s.sorted",
							 getenv("HOME"), GET_M(ti), GET_SEED(ti), GET_ALGO(ti) );
	return path;
}
#endif

long GET_FILE_SIZE( const char *path )
{
	FILE *f;
	long len;

	f = fopen( path, "rb" );
	if( ! f ) {
		return -1;
	}

	fseek( f, 0, SEEK_END );
	len = ftell( f );
	// rewind( f );

	fclose( f );

	return len;
}



typedef struct
{
	enum { NoMedium, File, Array } medium;
	
	int *array;
	long size;
} Data;

inline void destroyData( Data *data )
{
	if( data->medium == Array ) {
		data->medium = NoMedium;
		free( data->array );
	}
}



/*!
* main sorts data.
* this function will be implemented in several flavours (using several different
* algorithms or strategies), by a number of shared libraries dinamically loaded.
* This one is only called for the process with rank 0.
*/
void mainSort( const TestInfo *ti, Data *data );
typedef void (*MainSortFunction) ( const TestInfo *ti, Data data );
/*!
* sorts data.
* this function will be implemented in several flavours (using several different
* algorithms or strategies), by a number of shared libraries dinamically loaded.
* This one is called for any node but the one with rank 0.
*/
void sort( const TestInfo *ti );
typedef void (*SortFunction) ( const TestInfo *ti );



// typedef struct _Phase * PhaseHandle;
typedef int PhaseHandle;
/*!
* starts a phase (to study how much time it'll take
* returns the phase which needs to be stopped with stopPhase
*/
PhaseHandle startPhase( const TestInfo *ti, const char *phaseName );
/*!
* stops a phase
*/
void stopPhase( const TestInfo *ti, PhaseHandle phase );


void twoMerge ( Data*, Data*, Data* );


#if defined(__cplusplus)
}
#endif

#endif

