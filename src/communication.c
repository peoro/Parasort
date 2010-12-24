
#include "communication.h"

void send( Data *data, int dest )
{
	MPI_Send( data->array, data->size, MPI_INT, dest, 0, MPI_COMM_WORLD );
}
void receive( Data *data, long size, int source )
{
	allocDataArray( data, size );
	MPI_Recv( data->array, size, MPI_INT, source, 0, MPI_COMM_WORLD, NULL );
}

void scatterSend( TestInfo *ti, Data *data )
{
	MPI_Scatter( data->array, data->size / GET_N(ti), MPI_INT, NULL, 0, 0, GET_ID(ti), MPI_COMM_WORLD );
}
void scatterReceive( TestInfo *ti, Data *data, long size, int root )
{
	allocDataArray( data, size );
	MPI_Scatter( NULL, 0, 0, data->array, size, MPI_INT, root, MPI_COMM_WORLD );
}
void scatter( TestInfo *ti, Data *data, long size, int root )
{
	if( GET_ID(ti) == root ) {
		return scatterSend( ti, data );
	}
	else {
		return scatterReceive( ti, data, size, root );
	}
}