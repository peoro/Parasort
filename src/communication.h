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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <mpi.h>

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
long GET_M ( const TestInfo *ti );
long GET_LOCAL_M ( const TestInfo *ti );



typedef struct
{
	enum Medium {
		NoMedium = 0,
		File = 1,
		Array = 2
	} medium;

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

const char *DAL_mediumName( enum Medium m );
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
/********************************* [Integer] Communication Primitives ******************************************/
/***************************************************************************************************************/

void DAL_i_send( const TestInfo *ti, int **array, int size, int dest );
int DAL_i_receive( const TestInfo *ti, int **array, int size, int source );

int DAL_i_sendrecv( const TestInfo *ti, int **sarray, int scount, int sdispl, int **rarray, int rcount, int rdispl, int partner );

void DAL_i_scatterSend( const TestInfo *ti, int **array, int size );
void DAL_i_scatterReceive( const TestInfo *ti, int **array, int size, int root );
void DAL_i_scatter( const TestInfo *ti, int **array, int size, int root );

void DAL_i_scattervSend( const TestInfo *ti, int **array, int *sizes, int *displs );
void DAL_i_scattervReceive( const TestInfo *ti, int **array, int size, int root );
void DAL_i_scatterv( const TestInfo *ti, int **array, int *sizes, int *displs, int root );

void DAL_i_gatherSend( const TestInfo *ti, int **array, int size, int root );
void DAL_i_gatherReceive( const TestInfo *ti, int **array, int size );
void DAL_i_gather( const TestInfo *ti, int **array, int size, int root );

void DAL_i_gathervSend( const TestInfo *ti, int **array, int size, int root );
void DAL_i_gathervReceive( const TestInfo *ti, int **array, int *sizes, int *displs );
void DAL_i_gatherv( const TestInfo *ti, int **array, int *sizes, int *displs, int root );

void DAL_i_alltoall( const TestInfo *ti, int **array, int size );

void DAL_i_alltoallv( const TestInfo *ti, int **array, int *sendSizes, int *sdispls, int *recvSizes, int *rdispls );

void DAL_i_bcastSend( const TestInfo *ti, int **array, int size );
void DAL_i_bcastReceive( const TestInfo *ti, int **array, int size, int root );
void DAL_i_bcast( const TestInfo *ti, int **array, int size, int root );

/*--------------------------------------------------------------------------------------------------------------*/





/***************************************************************************************************************/
/*********************************** [Long] Communication Primitives *******************************************/
/***************************************************************************************************************/

void DAL_l_send( const TestInfo *ti, long **array, int size, int dest );
int DAL_l_receive( const TestInfo *ti, long **array, int size, int source );

int DAL_l_sendrecv( const TestInfo *ti, long **sarray, int scount, int sdispl, long **rarray, int rcount, int rdispl, int partner );

void DAL_l_scatterSend( const TestInfo *ti, long **array, int size );
void DAL_l_scatterReceive( const TestInfo *ti, long **array, int size, int root );
void DAL_l_scatter( const TestInfo *ti, long **array, int size, int root );

void DAL_l_scattervSend( const TestInfo *ti, long **array, int *sizes, int *displs );
void DAL_l_scattervReceive( const TestInfo *ti, long **array, int size, int root );
void DAL_l_scatterv( const TestInfo *ti, long **array, int *sizes, int *displs, int root );

void DAL_l_gatherSend( const TestInfo *ti, long **array, int size, int root );
void DAL_l_gatherReceive( const TestInfo *ti, long **array, int size );
void DAL_l_gather( const TestInfo *ti, long **array, int size, int root );

void DAL_l_gathervSend( const TestInfo *ti, long **array, int size, int root );
void DAL_l_gathervReceive( const TestInfo *ti, long **array, int *sizes, int *displs );
void DAL_l_gatherv( const TestInfo *ti, long **array, int *sizes, int *displs, int root );

void DAL_l_alltoall( const TestInfo *ti, long **array, int size );

void DAL_l_alltoallv( const TestInfo *ti, long **array, int *sendSizes, int *sdispls, int *recvSizes, int *rdispls );

void DAL_l_bcastSend( const TestInfo *ti, long **array, int size );
void DAL_l_bcastReceive( const TestInfo *ti, long **array, int size, int root );
void DAL_l_bcast( const TestInfo *ti, long **array, int size, int root );

/*--------------------------------------------------------------------------------------------------------------*/






/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
}
#endif


#endif

