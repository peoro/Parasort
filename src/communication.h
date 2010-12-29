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

void sendrecv( const TestInfo *ti, Data *sdata, long scount, long sdispl, Data* rdata, long rcount, long rdispl, int partner );

void scatterSend( const TestInfo *ti, Data *data );
void scatterReceive( const TestInfo *ti, Data *data, long size, int root );
void scatter( const TestInfo *ti, Data *data, long size, int root );

void scattervSend( const TestInfo *ti, Data *data, long *sizes, long *displs );
void scattervReceive( const TestInfo *ti, Data *data, long size, int root );
void scatterv( const TestInfo *ti, Data *data, long *sizes, long *displs, int root );


void gatherSend( const TestInfo *ti, Data *data, int root );
void gatherReceive( const TestInfo *ti, Data *data, long size );
void gather( const TestInfo *ti, Data *data, long size, int root );

void gathervSend( const TestInfo *ti, Data *data, int root );
void gathervReceive( const TestInfo *ti, Data *data, long *sizes, long *displs );
void gatherv( const TestInfo *ti, Data *data, long *sizes, long *displs, int root );

void alltoallv( const TestInfo *ti, Data *sendData, long *sendSizes, long *sdispls, long *recvSizes, long *rdispls );

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
}
#endif


#endif

