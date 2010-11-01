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
} TestInfo;


inline int GET_ID ( const TestInfo *ti ) {
    int x;
    MPI_Comm_rank ( MPI_COMM_WORLD, &x );
    return x;
}

inline int GET_N ( const TestInfo *ti ) {
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

inline char * GET_UNSORTED_DATA_PATH( const TestInfo *ti, char *path, int pathLen )
{
	snprintf( path, pathLen, "%s/.spd/data/id%d_n%d_M%ld_s%ld.unsorted",
							 getenv("HOME"), GET_ID(ti), GET_N(ti), GET_M(ti), GET_SEED(ti) );
	return path;
}
inline char * GET_SORTED_DATA_PATH( const TestInfo *ti, char *path, int pathLen )
{
	snprintf( path, pathLen, "%s/.spd/data/id%d_n%d_M%ld_s%ld.sorted",
							 getenv("HOME"), GET_ID(ti), GET_N(ti), GET_M(ti), GET_SEED(ti) );
	return path;
}

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
* sorts data.
* this function will be implemented in several flavours (using several different
* algorithms or strategies), by a number of shared libraries dinamically loaded.
*/
void sort( const TestInfo *ti, int *data, long size );
typedef void (*SortFunction) ( const TestInfo *ti, int *data, long size );

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

