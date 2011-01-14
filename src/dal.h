
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
			FILE *handle;
			char name[128];
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
        char SPD_BUF[1024]; \
        DAL_DEBUG( data, "WTF %s " fmt, DAL_dataItemsToString(data, SPD_BUF, sizeof(SPD_BUF)), ##__VA_ARGS__ ); \
	}

/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/*************************************** DAL file management functions *****************************************/
/***************************************************************************************************************/

long GET_FILE_SIZE( const char *path );

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

void DAL_init( Data *data ); // gets Data redy to be worked on - without allocating resources
void DAL_destroy( Data *data ); // destroys and re-initialize data

long DAL_dataSize( Data *data ); // returns data size, both of array or file...
bool DAL_allocData( Data *data, long size ); // allocates a Data in an Array, or, if it fails, in a File

// functions to work with any find of block device (ie: Files)
// TODO: to be removed
long DAL_readNextDeviceBlock( Data *device, Data *dst ); // reads a block of size of dst's moving cursor
void DAL_writeNextDeviceBlock( Data *device, Data *src ); // writes a block of size of src's at current cursor (moving it and overwriting current data)

// functions to work with any find of block device (ie: Files)
long DAL_readDataBlock( Data *data, long size, long dataOffset, Data *dst, long dstOffset ); // reads a block of size of dst's moving cursor
void DAL_writeDataBlock( Data *data, long size, long dataOffset, Data *src, long srcOffset ); // writes a block of size of src's at current cursor (moving it and overwriting current data)
	
// allocating an Array in memory
bool DAL_allocArray( Data *data, long size );
bool DAL_reallocArray ( Data *data, long size );
bool DAL_reallocAsArray( Data *data ); // tries to realloc data as an array

// allocating a temporary buffer in memory (an Array)
bool DAL_allocBuffer( Data *data, long size ); // allocs a buffer of size equal OR LESSER than size

// allocating a File
bool DAL_allocFile( Data *data, long size );

/*--------------------------------------------------------------------------------------------------------------*/


/***************************************************************************************************************/
/*************************************** DAL Communication Primitives ******************************************/
/***************************************************************************************************************/

void DAL_send( Data *data, int dest );
void DAL_receive( Data *data, long size, int source );

// A stands for Append: \data is already initialized, and received data is appended to it
// U stands for Unknown: \data size is unknown by receiver: sender will send it.
void DAL_receiveU( Data *data, int source );
void DAL_receiveA( Data *data, long size, int source );
void DAL_receiveAU( Data *data, int source );



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

/***************************************************************************************************************/
/***************************************** DAL Internal Functions **********************************************/
/***************************************************************************************************************/

// functions to work with any find of block device (ie: Files)
long DAL_deviceCursor( Data *device ); // gets current cursor position
void DAL_setDeviceCursor( Data *device, long pos ); // moves cursor position
void DAL_resetDeviceCursor( Data *device ); // moves cursor back to beginning


	
	
/*--------------------------------------------------------------------------------------------------------------*/


/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
}
#endif


#endif
