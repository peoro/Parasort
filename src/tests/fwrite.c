
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>

typedef uint64_t test_size_t;
#ifdef __cplusplus
	#define TSF "%lld" /* test_size_t format */
#else
	#define TSF "%"PRIu64 /* test_size_t format */ // WTF!?!? Why is this defined wrong?!? MPI SUCKS!!!
#endif

#ifdef __cplusplus
	const test_size_t Kilo = 1024;
	const test_size_t Mega = Kilo*Kilo;
	const test_size_t Giga = Mega*Kilo;
#else
	#define Kilo ((test_size_t)1024)
	#define Mega (Kilo*Kilo)
	#define Giga (Mega*Kilo)
#endif

test_size_t GET_FILE_SIZE( const char *path )
{
	FILE *f;
	test_size_t len;

	f = fopen( path, "rb" );
	if( ! f ) {
		return -1;
	}

	fseek( f, 0, SEEK_END );
	len = ftell( f );
	// rewind( f );

	fclose( f );

	return len;
}

int main( int argc, char **argv )
{
	if( argc != 2 ) {
		printf( "Usage: %s FILE\n", argv[0] );
		return 1;
	}
	
	FILE *f = fopen( argv[1], "w+" ); // same options as in DAL_allocFile
	if( ! f ) {
		printf( "fopen( %s, \"w+\" ) error: %s\n", argv[1], strerror(errno) );
		return 1;
	}
	
	// 1KB integers array to be written
	typedef int test_item_t;
#ifdef __cplusplus
	const test_size_t ItemSize = sizeof(test_item_t);
	const test_size_t ArraySize = Kilo;
	const test_size_t Items = ArraySize / ItemSize;
#else
	#define ItemSize ((test_size_t)sizeof(test_item_t))
	#define ArraySize ((test_size_t)Kilo)
	#define Items (ArraySize / ItemSize)
#endif
	
	test_item_t array[ Items ];
	test_size_t i;
	for( i = 0; i < Items; ++ i ) {
		array[i] = i;
	}
	
	// writing 10GB, 1KB per time
	test_size_t total = 0;
	
	for( i = 0; i < 10*Giga / ArraySize; ++ i ) { // 10M times
		test_size_t written = 0;
		while( written != Items ) {
			test_size_t current = fwrite( array + written, ItemSize, Items - written, f );
			if( ! current ) {
				printf( "fwrite( %p+"TSF", "TSF", "TSF"-"TSF", f ) error: %s\n", array, written, ItemSize, Items, written, strerror(errno) );
				fclose( f );
				return 1;
			}
			written += current;
		}
		
		total += written;
		printf( "Bytes written: "TSF"\tFile size: "TSF"\tDiff: "TSF"\n", total*ItemSize, GET_FILE_SIZE(argv[1]), total*ItemSize - GET_FILE_SIZE(argv[1]) );
	}
	
	fclose( f );
	
	return 0;
}
