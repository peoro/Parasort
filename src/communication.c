
#include<assert.h>
#include "communication.h"


void send( const TestInfo *ti, Data *data, int dest )
{
	MPI_Send( data->array, data->size, MPI_INT, dest, 0, MPI_COMM_WORLD );
}
void receive( const TestInfo *ti, Data *data, long size, int source )
{
	MPI_Status 	stat;
	assert( allocDataArray( data, size ) );
	MPI_Recv( data->array, size, MPI_INT, source, 0, MPI_COMM_WORLD, &stat );
}

void sendrecv( const TestInfo *ti, Data *sdata, long scount, long sdispl, Data* rdata, long rcount, long rdispl, int partner )
{
	int recvCount = rcount;
	int sendCount = scount;
	int recvDispl = rdispl;
	int sendDispl = sdispl;

	if ( recvDispl == 0 ) {
		assert( allocDataArray( rdata, recvCount ) );
	}
	else {
		assert( reallocDataArray( rdata, recvDispl+recvCount ) );
	}
	MPI_Status 	status;
	MPI_Sendrecv( sdata->array+sendDispl, sendCount, MPI_INT, partner, 100, rdata->array+recvDispl, recvCount, MPI_INT, partner, 100, MPI_COMM_WORLD, &status );

	MPI_Get_count( &status, MPI_INT, &recvCount );
	assert( reallocDataArray( rdata, recvDispl+recvCount ) );
}

void scatterSend( const TestInfo *ti, Data *data )
{
	assert( data->size % GET_N( ti ) == 0 );
	MPI_Scatter( data->array, data->size/GET_N( ti ), MPI_INT, MPI_IN_PLACE, data->size/GET_N( ti ), MPI_INT, GET_ID( ti ), MPI_COMM_WORLD );
	assert( reallocDataArray( data, data->size/GET_N( ti ) ) );
}
void scatterReceive( const TestInfo *ti, Data *data, long size, int root )
{
	assert( allocDataArray( data, size ) );
	MPI_Scatter( NULL, 0, MPI_INT, data->array, size, MPI_INT, root, MPI_COMM_WORLD );
}
void scatter( const TestInfo *ti, Data *data, long size, int root )
{
	if( GET_ID( ti ) == root ) {
		return scatterSend( ti, data );
	}
	else {
		return scatterReceive( ti, data, size, root );
	}
}

void gatherSend( const TestInfo *ti, Data *data, int root )
{
	MPI_Gather( data->array, data->size, MPI_INT, NULL, 0, MPI_INT, root, MPI_COMM_WORLD );
}
void gatherReceive( const TestInfo *ti, Data *data, long size )
{
	assert( size % GET_N( ti ) == 0 );
	assert( reallocDataArray( data, size ) );
	MPI_Gather( MPI_IN_PLACE, size/GET_N( ti ), MPI_INT, data->array, size/GET_N( ti ), MPI_INT, GET_ID( ti ), MPI_COMM_WORLD );
}
void gather( const TestInfo *ti, Data *data, long size, int root )
{
	if( GET_ID( ti ) == root ) {
		return gatherReceive( ti, data, size );
	}
	else {
		return gatherSend( ti, data, root );
	}
}

void scattervSend( const TestInfo *ti, Data *data, long *sizes, long *displs )
{
	int scounts[GET_N( ti )];
	int sdispls[GET_N( ti )];
	int i;

	for ( i=0; i<GET_N( ti ); i++ ) {
		scounts[i] = sizes[i];
		sdispls[i] = displs[i];
	}
 	MPI_Scatterv( data->array, scounts, sdispls, MPI_INT, MPI_IN_PLACE, sizes[0], MPI_INT, GET_ID( ti ), MPI_COMM_WORLD );
	assert( reallocDataArray( data, data->size/GET_N( ti ) ) );
}
void scattervReceive( const TestInfo *ti, Data *data, long size, int root )
{
	assert( allocDataArray( data, size ) );
 	MPI_Scatterv( NULL, NULL, NULL, MPI_INT, data->array, size, MPI_INT, root, MPI_COMM_WORLD );
}
void scatterv( const TestInfo *ti, Data *data, long *sizes, long *displs, int root )
{
	if( GET_ID( ti ) == root ) {
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
	int rcounts[GET_N( ti )];
	int rdispls[GET_N( ti )];
	int i;

	for ( i=0; i<GET_N( ti ); i++ ) {
		rcounts[i] = sizes[i];
		rdispls[i] = displs[i];
	}
	assert( reallocDataArray( data, GET_M( ti ) ) );
	MPI_Gatherv( MPI_IN_PLACE, rcounts[GET_ID( ti )], MPI_INT, data->array, rcounts, rdispls, MPI_INT, GET_ID( ti ), MPI_COMM_WORLD );
}
void gatherv( const TestInfo *ti, Data *data, long *sizes, long *displs, int root )
{
	if( GET_ID( ti ) == root ) {
		return gathervReceive( ti, data, sizes, displs );
	}
	else {
		return gathervSend( ti, data, root );
	}
}

void alltoallv( const TestInfo *ti, Data *data, long *sendSizes, long *sdispls, long *recvSizes, long *rdispls )
{
	int scounts[GET_N( ti )];
	int sd[GET_N( ti )];
	int rcounts[GET_N( ti )];
	int rd[GET_N( ti )];
	int rcount = 0;
	int i;

	for ( i=0; i<GET_N( ti ); i++ ) {
		scounts[i] = sendSizes[i];
		sd[i] = sdispls[i];

		rcounts[i] = recvSizes[i];
		rd[i] = rdispls[i];

		rcount += rcounts[i];
	}
	Data *recvData = (Data*)malloc( sizeof(Data) );

	assert( recvData != NULL && allocDataArray(recvData, rcount) );

	MPI_Alltoallv( data->array, scounts, sd, MPI_INT, recvData->array, rcounts, rd, MPI_INT, MPI_COMM_WORLD );

	destroyData( data );
	data->medium = Array;
	data->array = recvData->array;
	data->size = recvData->size;
}
