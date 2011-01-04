
/*
* This file re-defines part of the standard MPI API in order to being able to send data larger than 2^32 - 1 bytes.
* This constrain is due to the fact that functions such as MPI_Send, MPI_Recv, ..., uses an integer to represent the buffer size
*/

#ifndef _DAL_H_
#define _DAL_H_

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "debug.h"

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
extern "C" {
#endif


typedef enum DataMedium {
	NoMedium,
	File,
	Array
} DataMedium;

typedef struct Data
{
	DataMedium medium;

	union
	{
		struct
		{
			int *data;
			long size;
		} array;
		struct
		{
			char name[128];
		} file;
	};

} Data;


long GET_FILE_SIZE( const char *path );


const char *DAL_mediumName( DataMedium m );
char *DAL_dataToString( Data *d, char *s, int size );

#define UNSUPPORTED_DATA(d) \
	{ \
		char buf[1024]; \
		ERROR( "cannot handle data (%s)", DAL_dataToString((d), buf, sizeof(buf)) ); \
	}

#define ASSERT_DATA(cond, d, fmt, ... ) \
	if( ! (cond) ) { \
		char buf[1024]; \
		ERROR( "error with data (%s) " fmt, DAL_dataToString((d), buf, sizeof(buf)), ##__VA_ARGS__ ); \
	}


// TODO: make them more generic ...
bool DAL_isInitialized( Data *data );
void DAL_init( Data *data );
void DAL_destroy( Data *data );
bool DAL_allocArray( Data *data, int size );
bool DAL_reallocArray ( Data *data, int size ); // TODO: who needs this?



/***************************************************************************************************************/
/************************************* [Data] Communication Primitives *****************************************/
/***************************************************************************************************************/

void DAL_send( Data *data, int dest );
void DAL_receive( Data *data, long size, int source );

long DAL_sendrecv( Data *sdata, long scount, long sdispl, Data *rdata, long rcount, long rdispl, int partner );

void DAL_scatterSend( Data *data );
void DAL_scatterReceive( Data *data, long size, int root );
void DAL_scatter( Data *data, long size, int root );

void DAL_scattervSend( Data *data, long *sizes, long *displs );
void DAL_scattervReceive( Data *data, long size, int root );
void DAL_scatterv( Data *data, long *sizes, long *displs, int root );

void DAL_gatherSend( Data *data, int root );
void DAL_gatherReceive( Data *data, long size );
void DAL_gather( Data *data, long size, int root );

void DAL_gathervSend( Data *data, int root );
void DAL_gathervReceive( Data *data, long *sizes, long *displs );
void DAL_gatherv( Data *data, long *sizes, long *displs, int root );

void DAL_alltoall( Data *data, long size );

void DAL_alltoallv( Data *data, long *sendSizes, long *sdispls, long *recvSizes, long *rdispls );

void DAL_bcastSend( Data *data );
void DAL_bcastReceive( Data *data, long size, int root );
void DAL_bcast( Data *data, long size, int root );

/*--------------------------------------------------------------------------------------------------------------*/



/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
}
#endif


#endif
