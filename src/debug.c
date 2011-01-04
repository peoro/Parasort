
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// TODO: #if mpi is already included use it
// otherwise use system pid...
#include <mpi.h>
int SPD_getPid( )
{
    int x;
    MPI_Comm_rank ( MPI_COMM_WORLD, &x );
    return x;
}

const char *SPD_makeColor( int b, int fg, int bg )
{
	const char *colors[] = {
	"\E[0m\E[30m\E[40m",
	"\E[0m\E[30m\E[41m",
	"\E[0m\E[30m\E[42m",
	"\E[0m\E[30m\E[43m",
	"\E[0m\E[30m\E[44m",
	"\E[0m\E[30m\E[45m",
	"\E[0m\E[30m\E[46m",
	"\E[0m\E[30m\E[47m",
	"\E[0m\E[31m\E[40m",
	"\E[0m\E[31m\E[41m",
	"\E[0m\E[31m\E[42m",
	"\E[0m\E[31m\E[43m",
	"\E[0m\E[31m\E[44m",
	"\E[0m\E[31m\E[45m",
	"\E[0m\E[31m\E[46m",
	"\E[0m\E[31m\E[47m",
	"\E[0m\E[32m\E[40m",
	"\E[0m\E[32m\E[41m",
	"\E[0m\E[32m\E[42m",
	"\E[0m\E[32m\E[43m",
	"\E[0m\E[32m\E[44m",
	"\E[0m\E[32m\E[45m",
	"\E[0m\E[32m\E[46m",
	"\E[0m\E[32m\E[47m",
	"\E[0m\E[33m\E[40m",
	"\E[0m\E[33m\E[41m",
	"\E[0m\E[33m\E[42m",
	"\E[0m\E[33m\E[43m",
	"\E[0m\E[33m\E[44m",
	"\E[0m\E[33m\E[45m",
	"\E[0m\E[33m\E[46m",
	"\E[0m\E[33m\E[47m",
	"\E[0m\E[34m\E[40m",
	"\E[0m\E[34m\E[41m",
	"\E[0m\E[34m\E[42m",
	"\E[0m\E[34m\E[43m",
	"\E[0m\E[34m\E[44m",
	"\E[0m\E[34m\E[45m",
	"\E[0m\E[34m\E[46m",
	"\E[0m\E[34m\E[47m",
	"\E[0m\E[35m\E[40m",
	"\E[0m\E[35m\E[41m",
	"\E[0m\E[35m\E[42m",
	"\E[0m\E[35m\E[43m",
	"\E[0m\E[35m\E[44m",
	"\E[0m\E[35m\E[45m",
	"\E[0m\E[35m\E[46m",
	"\E[0m\E[35m\E[47m",
	"\E[0m\E[36m\E[40m",
	"\E[0m\E[36m\E[41m",
	"\E[0m\E[36m\E[42m",
	"\E[0m\E[36m\E[43m",
	"\E[0m\E[36m\E[44m",
	"\E[0m\E[36m\E[45m",
	"\E[0m\E[36m\E[46m",
	"\E[0m\E[36m\E[47m",
	"\E[0m\E[37m\E[40m",
	"\E[0m\E[37m\E[41m",
	"\E[0m\E[37m\E[42m",
	"\E[0m\E[37m\E[43m",
	"\E[0m\E[37m\E[44m",
	"\E[0m\E[37m\E[45m",
	"\E[0m\E[37m\E[46m",
	"\E[0m\E[37m\E[47m",
	"\E[1m\E[30m\E[40m",
	"\E[1m\E[30m\E[41m",
	"\E[1m\E[30m\E[42m",
	"\E[1m\E[30m\E[43m",
	"\E[1m\E[30m\E[44m",
	"\E[1m\E[30m\E[45m",
	"\E[1m\E[30m\E[46m",
	"\E[1m\E[30m\E[47m",
	"\E[1m\E[31m\E[40m",
	"\E[1m\E[31m\E[41m",
	"\E[1m\E[31m\E[42m",
	"\E[1m\E[31m\E[43m",
	"\E[1m\E[31m\E[44m",
	"\E[1m\E[31m\E[45m",
	"\E[1m\E[31m\E[46m",
	"\E[1m\E[31m\E[47m",
	"\E[1m\E[32m\E[40m",
	"\E[1m\E[32m\E[41m",
	"\E[1m\E[32m\E[42m",
	"\E[1m\E[32m\E[43m",
	"\E[1m\E[32m\E[44m",
	"\E[1m\E[32m\E[45m",
	"\E[1m\E[32m\E[46m",
	"\E[1m\E[32m\E[47m",
	"\E[1m\E[33m\E[40m",
	"\E[1m\E[33m\E[41m",
	"\E[1m\E[33m\E[42m",
	"\E[1m\E[33m\E[43m",
	"\E[1m\E[33m\E[44m",
	"\E[1m\E[33m\E[45m",
	"\E[1m\E[33m\E[46m",
	"\E[1m\E[33m\E[47m",
	"\E[1m\E[34m\E[40m",
	"\E[1m\E[34m\E[41m",
	"\E[1m\E[34m\E[42m",
	"\E[1m\E[34m\E[43m",
	"\E[1m\E[34m\E[44m",
	"\E[1m\E[34m\E[45m",
	"\E[1m\E[34m\E[46m",
	"\E[1m\E[34m\E[47m",
	"\E[1m\E[35m\E[40m",
	"\E[1m\E[35m\E[41m",
	"\E[1m\E[35m\E[42m",
	"\E[1m\E[35m\E[43m",
	"\E[1m\E[35m\E[44m",
	"\E[1m\E[35m\E[45m",
	"\E[1m\E[35m\E[46m",
	"\E[1m\E[35m\E[47m",
	"\E[1m\E[36m\E[40m",
	"\E[1m\E[36m\E[41m",
	"\E[1m\E[36m\E[42m",
	"\E[1m\E[36m\E[43m",
	"\E[1m\E[36m\E[44m",
	"\E[1m\E[36m\E[45m",
	"\E[1m\E[36m\E[46m",
	"\E[1m\E[36m\E[47m",
	"\E[1m\E[37m\E[40m",
	"\E[1m\E[37m\E[41m",
	"\E[1m\E[37m\E[42m",
	"\E[1m\E[37m\E[43m",
	"\E[1m\E[37m\E[44m",
	"\E[1m\E[37m\E[45m",
	"\E[1m\E[37m\E[46m",
	"\E[1m\E[37m\E[47]"
	};
	
	// printf( "%d,%d,%d :: %d\n", b, fg, bg, b*8*8 + (fg%8)*8 + bg );
	return colors[ b*8*8 + (fg%8)*8 + bg%8 ];
}

#include <execinfo.h>
void print_trace( void )
{
	void *array[64];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace( array, 64 );
	strings = backtrace_symbols( array, size );

	printf( "\n" );
	printf( "Tracebak (%zd stack frames):\n", size );

	for( i = 0; i < size; ++ i ) {
		printf( "  %s\n", strings[i] );
	}
	printf( "\n" );

	free( strings );
}

