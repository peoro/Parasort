/*
* This file re-defines part of the standard MPI API in order to being able to send data larger than 2^32 - 1 bytes.
* This constrain is due to the fact that functions such as MPI_Send, MPI_Recv, ..., uses an integer to represent the buffer size
*/

#ifndef _LONGMPI_H_
#define _LONGMPI_H_

#include <mpi.h>

int _MPI_Get_count( MPI_Status *status, MPI_Datatype datatype, long *count );
int _MPI_Recv( void *buf, long count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status );
int _MPI_Send( void *buf, long count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm );
int _MPI_Bcast ( void *buffer, long count, MPI_Datatype datatype, int root, MPI_Comm comm );
int _MPI_Scatter ( void *sendbuf, long sendcnt, MPI_Datatype sendtype, void *recvbuf, long recvcnt, MPI_Datatype recvtype, int root, MPI_Comm comm );
int _MPI_Gather ( void *sendbuf, long sendcnt, MPI_Datatype sendtype, void *recvbuf, long recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm );
int _MPI_Alltoall( void *sendbuf, long sendcount, MPI_Datatype sendtype, void *recvbuf, long recvcnt, MPI_Datatype recvtype, MPI_Comm comm );

#endif

