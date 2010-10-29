#ifndef _SORTING_H_
#define _SORTING_H_

#include "mpi.h"

//coppola's: 3483962622


typedef struct {
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




/*!
* sorta  
*/
void sort ( int *data, int size );

/*!
* generate the unsorted data file for the current scenario 
*/
void generate ( );

/*!
* apre il file il cui nome è ricavato da f(M,seed)  
*/
void loadData ( );

/*!
* dato l'array data, ne salva il contenuto in un file il cui nome è g(M,seed)
*/
void storeData ( int *data, int size );


#endif