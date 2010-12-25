
#include "communication.h"

void send( const TestInfo *ti, Data *data, int dest )
{
	MPI_Send( data->array, data->size, MPI_INT, dest, 0, MPI_COMM_WORLD );
}
void receive( const TestInfo *ti, Data *data, long size, int source )
{
	allocDataArray( data, size );
	MPI_Recv( data->array, size, MPI_INT, source, 0, MPI_COMM_WORLD, NULL );
}

void scatterSend( const TestInfo *ti, Data *data )
{
	MPI_Scatter( data->array, data->size / GET_N(ti), MPI_INT, MPI_IN_PLACE, 0, 0, GET_ID(ti), MPI_COMM_WORLD );
}
void scatterReceive( const TestInfo *ti, Data *data, long size, int root )
{
	allocDataArray( data, size );
	MPI_Scatter( NULL, 0, 0, data->array, size, MPI_INT, root, MPI_COMM_WORLD );
}
void scatter( const TestInfo *ti, Data *data, long size, int root )
{
	if( GET_ID(ti) == root ) {
		return scatterSend( ti, data );
	}
	else {
		return scatterReceive( ti, data, size, root );
	}
}


void scattervSend( const TestInfo *ti, Data *data, long *sizes, long *displs )
{
	int scounts[GET_N(ti)];
	int sdispls[GET_N(ti)];
	int i;

	for ( i=0; i<GET_N(ti); i++ ) {
		scounts[i] = sizes[i];
		sdispls[i] = displs[i];
	}
 	MPI_Scatterv( data->array, scounts, sdispls, MPI_INT, MPI_IN_PLACE, sizes[0], MPI_INT, GET_ID(ti), MPI_COMM_WORLD );
}
void scattervReceive( const TestInfo *ti, Data *data, long size, int root )
{
	allocDataArray( data, size );
 	MPI_Scatterv( NULL, NULL, NULL, MPI_INT, data->array, size, MPI_INT, root, MPI_COMM_WORLD );
}
void scatterv( const TestInfo *ti, Data *data, long *sizes, long *displs, int root )
{
	if( GET_ID(ti) == root ) {
		return scattervSend( ti, data, sizes, displs );
	}
	else {
		return scattervReceive( ti, data, GET_LOCAL_M(ti), root );
	}
}

void gathervSend( const TestInfo *ti, Data *data, int root )
{
	MPI_Gatherv( data->array, data->size, MPI_INT, NULL, NULL, NULL, MPI_INT, root, MPI_COMM_WORLD );
}
void gathervReceive( const TestInfo *ti, Data *data, long *sizes, long *displs )
{
	int rcounts[GET_N(ti)];
	int rdispls[GET_N(ti)];
	int i;

	for ( i=0; i<GET_N(ti); i++ ) {
		rcounts[i] = sizes[i];
		rdispls[i] = displs[i];
	}
	MPI_Gatherv( MPI_IN_PLACE, rcounts[GET_ID(ti)], MPI_INT, data->array, rcounts, rdispls, MPI_INT, GET_ID(ti), MPI_COMM_WORLD );
}
void gatherv( const TestInfo *ti, Data *data, long *sizes, long *displs, int root )
{
	if( GET_ID(ti) == root ) {
		return gathervReceive( ti, data, sizes, displs );
	}
	else {
		return gathervSend( ti, data, root );
	}
}

void alltoallv( const TestInfo *ti, Data *sendData, long *sendSizes, long *sdispls, Data *recvData, long *recvSizes, long *rdispls )
{
	int scounts[GET_N(ti)];
	int sd[GET_N(ti)];
	int rcounts[GET_N(ti)];
	int rd[GET_N(ti)];
	int i;

	for ( i=0; i<GET_N(ti); i++ ) {
		scounts[i] = sendSizes[i];
		sd[i] = sdispls[i];
		rcounts[i] = recvSizes[i];
		rd[i] = rdispls[i];
	}
	MPI_Alltoallv( sendData->array, scounts, sd, MPI_INT, recvData->array, rcounts, rd, MPI_INT, MPI_COMM_WORLD );
}