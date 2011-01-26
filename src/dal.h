
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

#define DST "%lld"
typedef long long dal_size_t;

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
			dal_size_t size;
		} array;
		struct
		{
			FILE *handle;
			char name[128];
			dal_size_t size;
		} file;
	};

} Data;



/***************************************************************************************************************/
/****************************************** DAL debugging functions ********************************************/
/***************************************************************************************************************/

const char *DAL_mediumName( DataMedium m );
char *DAL_dataToString( Data *d, char *s, int size );
char *DAL_dataItemsToString( Data *d, char *s, int size );


#define DAL_UNSUPPORTED(d) \
	{ \
		char SPD_BUF[1024]; \
		SPD_ERROR( "cannot handle data (%s)", DAL_dataToString((d), SPD_BUF, sizeof(SPD_BUF)) ); \
	}

#define DAL_UNIMPLEMENTED(d) \
	{ \
		char SPD_BUF[1024]; \
		SPD_ERROR( "support for data (%s) not yet implemented", DAL_dataToString((d), SPD_BUF, sizeof(SPD_BUF)) ); \
	}

#define DAL_ASSERT(cond, d, fmt, ... ) \
	if( ! (cond) ) { \
		char SPD_BUF[1024]; \
		SPD_ASSERT( (cond), "error with data (%s) " fmt, DAL_dataToString((d), SPD_BUF, sizeof(SPD_BUF)), ##__VA_ARGS__ ); \
	}

#define DAL_ASSERT2(cond, d1, d2, fmt, ... ) \
	if( ! (cond) ) { \
		char SPD_BUF[1024]; \
		SPD_ASSERT( (cond), "error with data (%s) " fmt, DAL_dataToString((d1), SPD_BUF, sizeof(SPD_BUF)), ##__VA_ARGS__ ); \
	}


#define DAL_ERROR(d, fmt, ... ) \
	{ \
		char SPD_BUF[1024]; \
		SPD_ERROR( "data (%s) " fmt, DAL_dataToString((d), SPD_BUF, sizeof(SPD_BUF)), ##__VA_ARGS__ ); \
	}

#define DAL_DEBUG(d, fmt, ... ) \
	{ \
		char SPD_BUF[1024]; \
		SPD_DEBUG( "data (%s) " fmt, DAL_dataToString((d), SPD_BUF, sizeof(SPD_BUF)), ##__VA_ARGS__ ); \
	}

#define DAL_PRINT_DATA(d, fmt, ... ) \
	{ \
        char SPD_BUFx[64]; \
        DAL_DEBUG( (d), " %s " fmt, DAL_dataItemsToString((d), SPD_BUFx, sizeof(SPD_BUFx)), ##__VA_ARGS__ ); \
	}

/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/********************************************* DAL misc functions **********************************************/
/***************************************************************************************************************/

void DAL_initialize( int *argc, char ***argv );
void DAL_finalize( );

/***************************************************************************************************************/
/******************************************** DAL Data management **********************************************/
/***************************************************************************************************************/

bool DAL_isInitialized( Data *data );

dal_size_t DAL_dataSize( Data *data ); // returns data size, both of array or file...

void DAL_init( Data *data ); // gets Data redy to be worked on - without allocating resources
void DAL_destroy( Data *data ); // destroys and re-initialize data

bool DAL_allocData( Data *data, dal_size_t size ); // allocates a Data in an Array, or, if it fails, in a File
bool DAL_reallocData ( Data *data, dal_size_t size );

dal_size_t DAL_allowedBufSize( );

// swaps two datas: \post a == \b, \post b == \a
void DAL_dataSwap( Data *a, Data *b );

/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/*************************************** DAL Communication Primitives ******************************************/
/***************************************************************************************************************/

// send receive
void DAL_send( Data *data, int dest );
void DAL_receive( Data *data, dal_size_t size, int source );

// A stands for Append: \data is already initialized, and received data is appended to it
// U stands for Unknown: \data size is unknown by receiver: sender will send it.
void DAL_sendU( Data *data, int dest );
void DAL_receiveU( Data *data, int source );
void DAL_receiveA( Data *data, dal_size_t size, int source );
void DAL_receiveAU( Data *data, int source );

dal_size_t DAL_sendrecv( Data *sdata, dal_size_t scount, dal_size_t sdispl, Data *rdata, dal_size_t rcount, dal_size_t rdispl, int partner );

// scatter
void DAL_scatterSend( Data *data );
void DAL_scatterReceive( Data *data, dal_size_t count, int root );
void DAL_scatter( Data *data, dal_size_t count, int root );

void DAL_scattervSend( Data *data, dal_size_t *sizes, dal_size_t *displs );
void DAL_scattervReceive( Data *data, dal_size_t count, int root );
void DAL_scatterv( Data *data, dal_size_t *sizes, dal_size_t *displs, int root );

// gather
void DAL_gatherSend( Data *data, int root );
void DAL_gatherReceive( Data *data, dal_size_t size );
void DAL_gather( Data *data, dal_size_t size, int root );

void DAL_gathervSend( Data *data, int root );
void DAL_gathervReceive( Data *data, dal_size_t *sizes, dal_size_t *displs );
void DAL_gatherv( Data *data, dal_size_t *sizes, dal_size_t *displs, int root );

// all to all
void DAL_alltoall( Data *data, dal_size_t count );

void DAL_alltoallv( Data *data, dal_size_t *sendSizes, dal_size_t *sdispls, dal_size_t *recvSizes, dal_size_t *rdispls );

// broadcast
void DAL_bcastSend( Data *data );
void DAL_bcastReceive( Data *data, dal_size_t size, int root );
void DAL_bcast( Data *data, dal_size_t size, int root );



/*--------------------------------------------------------------------------------------------------------------*/


/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
}
#endif


#endif
