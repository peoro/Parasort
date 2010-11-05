#ifndef _SORTING_H_
#define _SORTING_H_

#include <mpi.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

//coppola's: 3483962622


typedef struct
{
	bool verbose;
	bool threaded; // use multithread between cores

    long M; //data count
    long seed;
    char algo[64];
    int algoVar;
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
	snprintf( path, pathLen, "%s/.spd/data/M%ld_s%ld.sorted",
							 getenv("HOME"), GET_M(ti), GET_SEED(ti) );
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
	snprintf( path, pathLen, "%s/.spd/data/M%ld_s%ld_human.sorted",
							 getenv("HOME"), GET_M(ti), GET_SEED(ti) );
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



/*!
* main sorts data.
* this function will be implemented in several flavours (using several different
* algorithms or strategies), by a number of shared libraries dinamically loaded.
* This one is only called for the process with rank 0.
*/
void mailSort( const TestInfo *ti, int *data, long size );
typedef void (*MainSortFunction) ( const TestInfo *ti, int *data, long size );
/*!
* sorts data.
* this function will be implemented in several flavours (using several different
* algorithms or strategies), by a number of shared libraries dinamically loaded.
* This one is called for any node but the one with rank 0.
*/
void sort( const TestInfo *ti );
typedef void (*SortFunction) ( const TestInfo *ti );

/*!
* generates the unsorted data file for the current scenario
*/
int generate( const TestInfo *ti );

/*!
* loads unsorted data file
*/
int loadData( const TestInfo *ti, int **data, long *size );

/*!
* stores the result to our sorted data file
*/
int storeData( const TestInfo *ti, int *data, long size );


#endif

