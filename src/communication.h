
/*
* This file re-defines part of the standard MPI API in order to being able to send data larger than 2^32 - 1 bytes.
* This constrain is due to the fact that functions such as MPI_Send, MPI_Recv, ..., uses an integer to represent the buffer size
*/

#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
extern "C" {
#endif

// TODO: this should stay in sorting.h, which is at a higher level than communication
// but communication functions are using TestInfo... hm...
typedef struct
{
	bool verbose;
	bool threaded; // use multithread between cores

    long M; //data count
    long seed;
    char algo[64];
    int algoVar[3];
} TestInfo;

int GET_ID ( const TestInfo *ti );
int GET_N ( const TestInfo *ti );

typedef enum {
	DAL_CHAR,
	DAL_SIGNED_CHAR,
	DAL_UNSIGNED_CHAR,
	DAL_BYTE,
	DAL_WCHAR,
	DAL_SHORT,
	DAL_UNSIGNED_SHORT,
	DAL_INT,
	DAL_UNSIGNED,
	DAL_LONG,
	DAL_UNSIGNED_LONG,
	DAL_FLOAT,
	DAL_DOUBLE,
	DAL_LONG_DOUBLE,
	DAL_LONG_LONG_INT,
	DAL_LONG_LONG,
	DAL_UNSIGNED_LONG_LONG
} DAL_Type;

typedef enum DataMedium {
	NoMedium = 0,
	File = 1,
	Array = 2
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


// debugging stuff ...


void print_trace( void ); // TODO: pure debugging, move in debug.c ...
#define COLOR_DEFAULT	"\E[0m"
#define COLOR_RED		"\E[1m\E[31m"
#define COLOR_GREEN		"\E[1m\E[32m"
#define COLOR_YELLOW	"\E[1m\E[33m"
#define ASSERT(x) if( ! (x) ) { print_trace(); exit(1); }


long GET_FILE_SIZE( const char *path );

const char *DAL_mediumName( DataMedium m );
char *DAL_dataToString( Data *d, char *s, int size );

#define BASE_ERROR_STR COLOR_YELLOW "%s:%d" COLOR_DEFAULT ": " COLOR_RED "%s()" COLOR_DEFAULT ": "
#define BASE_ERROR_ARGS __FILE__, __LINE__, __FUNCTION__

#define UNSUPPORTED_DATA(d) \
	{ \
		char buf[1024]; \
		printf( BASE_ERROR_STR "cannot handle data (%s).\n", BASE_ERROR_ARGS, DAL_dataToString((d), buf, sizeof(buf)) ); \
		ASSERT(false); \
	}

#define ASSERT_DATA(cond, d, s) \
	if( ! (cond) ) { \
		char buf[1024]; \
		printf( BASE_ERROR_STR "error with data (%s), %s.\n", BASE_ERROR_ARGS, DAL_dataToString((d), buf, sizeof(buf)), (s) ); \
		ASSERT(false); \
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

void DAL_send( const TestInfo *ti, Data *data, int dest );
void DAL_receive( const TestInfo *ti, Data *data, long size, int source );

long DAL_sendrecv( const TestInfo *ti, Data *sdata, long scount, long sdispl, Data *rdata, long rcount, long rdispl, int partner );

void DAL_scatterSend( const TestInfo *ti, Data *data );
void DAL_scatterReceive( const TestInfo *ti, Data *data, long size, int root );
void DAL_scatter( const TestInfo *ti, Data *data, long size, int root );

void DAL_scattervSend( const TestInfo *ti, Data *data, long *sizes, long *displs );
void DAL_scattervReceive( const TestInfo *ti, Data *data, long size, int root );
void DAL_scatterv( const TestInfo *ti, Data *data, long *sizes, long *displs, int root );

void DAL_gatherSend( const TestInfo *ti, Data *data, int root );
void DAL_gatherReceive( const TestInfo *ti, Data *data, long size );
void DAL_gather( const TestInfo *ti, Data *data, long size, int root );

void DAL_gathervSend( const TestInfo *ti, Data *data, int root );
void DAL_gathervReceive( const TestInfo *ti, Data *data, long *sizes, long *displs );
void DAL_gatherv( const TestInfo *ti, Data *data, long *sizes, long *displs, int root );

void DAL_alltoall( const TestInfo *ti, Data *data, long size );

void DAL_alltoallv( const TestInfo *ti, Data *data, long *sendSizes, long *sdispls, long *recvSizes, long *rdispls );

void DAL_bcastSend( const TestInfo *ti, Data *data );
void DAL_bcastReceive( const TestInfo *ti, Data *data, long size, int root );
void DAL_bcast( const TestInfo *ti, Data *data, long size, int root );

/*--------------------------------------------------------------------------------------------------------------*/





/***************************************************************************************************************/
/********************************* [DAL_Type] Communication Primitives *****************************************/
/***************************************************************************************************************/

void DAL_type_send( const TestInfo *ti, void *array, int size, DAL_Type type, int dest );
int DAL_type_receive( const TestInfo *ti, void *array, int size, DAL_Type type, int source );

int DAL_type_sendrecv( const TestInfo *ti, void *sarray, int scount, void *rarray, int rcount, DAL_Type type, int partner );

void DAL_type_scatterSend( const TestInfo *ti, void *array, int size, DAL_Type type );
void DAL_type_scatterReceive( const TestInfo *ti, void *array, int size, DAL_Type type, int root );
void DAL_type_scatter( const TestInfo *ti, void *array, int size, DAL_Type type, int root );

void DAL_type_scattervSend( const TestInfo *ti, void *array, int *sizes, int *displs, DAL_Type type );
void DAL_type_scattervReceive( const TestInfo *ti, void *array, int size, DAL_Type type, int root );
void DAL_type_scatterv( const TestInfo *ti, void *array, int *sizes, int *displs, DAL_Type type, int root );

void DAL_type_gatherSend( const TestInfo *ti, void *array, int size, DAL_Type type, int root );
void DAL_type_gatherReceive( const TestInfo *ti, void *array, int size, DAL_Type type );
void DAL_type_gather( const TestInfo *ti, void *array, int size, DAL_Type type, int root );

void DAL_type_gathervSend( const TestInfo *ti, void *array, int size, DAL_Type type, int root );
void DAL_type_gathervReceive( const TestInfo *ti, void *array, int *sizes, int *displs, DAL_Type type );
void DAL_type_gatherv( const TestInfo *ti, void *array, int *sizes, int *displs, DAL_Type type, int root );

void DAL_type_alltoall( const TestInfo *ti, void *sarray, void *rarray, int size, DAL_Type type );

void DAL_type_alltoallv( const TestInfo *ti, void *sarray, int *sendSizes, int *sdispls, void *rarray, int *recvSizes, int *rdispls, DAL_Type type );

void DAL_type_bcastSend( const TestInfo *ti, void *array, int size, DAL_Type type );
void DAL_type_bcastReceive( const TestInfo *ti, void *array, int size, DAL_Type type, int root );
void DAL_type_bcast( const TestInfo *ti, void *array, int size, DAL_Type type, int root );

/*--------------------------------------------------------------------------------------------------------------*/




/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
}
#endif


#endif
