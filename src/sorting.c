#include "sorting.h"
#include <mpi.h>

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
extern "C" {
#endif

int GET_ID ( const TestInfo *ti ) {
	(void) ti;
    int x;
    MPI_Comm_rank ( MPI_COMM_WORLD, &x );
    return x;
}

int GET_N ( const TestInfo *ti ) {
	(void) ti;
    int x;
    MPI_Comm_size ( MPI_COMM_WORLD, &x );
    return x;
}

dal_size_t GET_M ( const TestInfo *ti ) {
    return ti->M;
}

dal_size_t GET_LOCAL_M ( const TestInfo *ti ) {
    dal_size_t div = GET_M(ti) / GET_N(ti);
    int rem = GET_M(ti) % GET_N(ti);
    return div + ( GET_ID(ti) < rem );
}

const char* GET_ALGO ( const TestInfo *ti ) {
    return ti->algo;
}

dal_size_t GET_SEED ( const TestInfo *ti ) {
    return ti->seed;
}

char * GET_ALGORITHM_PATH( const char *algo, char *path, int pathLen )
{
	snprintf( path, pathLen, "%s/.spd/algorithms/lib%s.so", getenv("HOME"), algo );
	return path;
}

#ifndef DEBUG
char * GET_UNSORTED_DATA_PATH( const TestInfo *ti, char *path, int pathLen )
{
	snprintf( path, pathLen, "%s/.spd/data/M"DST"_s"DST".unsorted",
							 getenv("HOME"), GET_M(ti), GET_SEED(ti) );
	return path;
}
char * GET_SORTED_DATA_PATH( const TestInfo *ti, char *path, int pathLen )
{
	snprintf( path, pathLen, "%s/.spd/data/M"DST"_s"DST".%s.sorted",
							 getenv("HOME"), GET_M(ti), GET_SEED(ti), GET_ALGO(ti) );
	return path;
}
#else
char * GET_UNSORTED_DATA_PATH( const TestInfo *ti, char *path, int pathLen )
{
	snprintf( path, pathLen, "%s/.spd/data/M"DST"_s"DST"_human.unsorted",
							 getenv("HOME"), GET_M(ti), GET_SEED(ti) );
	return path;
}
char * GET_SORTED_DATA_PATH( const TestInfo *ti, char *path, int pathLen )
{
	snprintf( path, pathLen, "%s/.spd/data/M"DST"_s"DST"_human.%s.sorted",
							 getenv("HOME"), GET_M(ti), GET_SEED(ti), GET_ALGO(ti) );
	return path;
}
#endif

#if defined(__cplusplus)
}
#endif


