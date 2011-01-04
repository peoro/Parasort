#ifndef _SORTING_H_
#define _SORTING_H_

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "communication.h"

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
extern "C" {
#endif



long GET_M ( const TestInfo *ti );
long GET_LOCAL_M ( const TestInfo *ti );
const char* GET_ALGO ( const TestInfo *ti );
long GET_SEED ( const TestInfo *ti );
char * GET_ALGORITHM_PATH( const char *algo, char *path, int pathLen );

#ifndef DEBUG
char * GET_UNSORTED_DATA_PATH( const TestInfo *ti, char *path, int pathLen );
char * GET_SORTED_DATA_PATH( const TestInfo *ti, char *path, int pathLen );
#else
char * GET_UNSORTED_DATA_PATH( const TestInfo *ti, char *path, int pathLen );
char * GET_SORTED_DATA_PATH( const TestInfo *ti, char *path, int pathLen );
#endif


/*!
* main sorts data.
* this function will be implemented in several flavours (using several different
* algorithms or strategies), by a number of shared libraries dinamically loaded.
* This one is only called for the process with rank 0.
*/
void mainSort( const TestInfo *ti, Data *data );
typedef void (*MainSortFunction) ( const TestInfo *ti, Data *data );
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

#if defined(__cplusplus)
}
#endif

#endif

