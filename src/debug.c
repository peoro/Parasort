
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

