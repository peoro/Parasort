/*
* This file re-defines part of the standard MPI API in order to being able to send data larger than 2^32 - 1 bytes.
* This constrain is due to the fact that functions such as MPI_Send, MPI_Recv, ..., uses an integer to represent the buffer size
*/

#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
extern "C" {
#endif

#include <mpi.h>
#include "sorting.h"

/*
// functions to re-implement
int _MPI_Bcast ( void *buffer, long count, MPI_Datatype datatype, int root, MPI_Comm comm );
int _MPI_Scatter ( void *sendbuf, long sendcnt, MPI_Datatype sendtype, void *recvbuf, long recvcnt, MPI_Datatype recvtype, int root, MPI_Comm comm );
int _MPI_Alltoall( void *sendbuf, long sendcount, MPI_Datatype sendtype, void *recvbuf, long recvcnt, MPI_Datatype recvtype, MPI_Comm comm );
+ scatterv, sendU, receiveU, scatterU, scattervU ...
*/

void send( const TestInfo *ti, Data *data, int dest );
void receive( const TestInfo *ti, Data *data, long size, int source );

void scatterSend( const TestInfo *ti, Data *data );
void scatterReceive( const TestInfo *ti, Data *data, long size, int root );
void scatter( const TestInfo *ti, Data *data, long size, int root );

/*
scattervSend( Data *data, int *sizes, int *displacements );
scattervReceive( Data *data, int size, int root );
scatterv( Data *data, int *sizes, int *displacements, int root );
*/

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
}
#endif


#endif

