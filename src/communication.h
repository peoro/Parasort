/*
* This file re-defines part of the standard MPI API in order to being able to send data larger than 2^32 - 1 bytes.
* This constrain is due to the fact that functions such as MPI_Send, MPI_Recv, ..., uses an integer to represent the buffer size
*/

#ifndef _LONGMPI_H_
#define _LONGMPI_H_

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
extern "C" {
#endif

#include <mpi.h>

/*
// functions to re-implement
int _MPI_Bcast ( void *buffer, long count, MPI_Datatype datatype, int root, MPI_Comm comm );
int _MPI_Scatter ( void *sendbuf, long sendcnt, MPI_Datatype sendtype, void *recvbuf, long recvcnt, MPI_Datatype recvtype, int root, MPI_Comm comm );
int _MPI_Alltoall( void *sendbuf, long sendcount, MPI_Datatype sendtype, void *recvbuf, long recvcnt, MPI_Datatype recvtype, MPI_Comm comm );
+ scatterv, sendU, receiveU, scatterU, scattervU ...
*/

inline void send( Data *data, int dest )
{
	MPI_Send( data->array, data->size, MPI_INT, dest, 0, MPI_COMM_WORLD );
}
inline void receive( Data *data, long size, int source )
{
	allocArray( data, size );
	MPI_Recv( data->array, size, MPI_INT, source, 0, MPI_COMM_WORLD );
}

inline void scatterSend( TestInfo *ti, Data *data )
{
	MPI_Scatter( data->array, data->size / GET_N(ti), MPI_INT, NULL, 0, 0, GET_ID(ti), MPI_COMM_WORLD );
}
inline void scatterReceive( TestInfo *ti, Data *data, long size, int root )
{
	MPI_Scatter( NULL, 0, 0, data->array, size, MPI_INT, root, MPI_COMM_WORLD );
}
inline void scatter( TestInfo *ti, Data *data, long size, int root )
{
	if( GET_ID(ti) == root ) {
		return scatterSend( ti, data );
	}
	else {
		return scatterReceive( ti, data, size, root );
	}
}

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

