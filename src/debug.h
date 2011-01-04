

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include<assert.h>

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
extern "C" {
#endif


void print_trace( void ); // TODO: pure debugging, move in debug.c ...

#define COLOR_DEFAULT	"\E[0m"
#define COLOR_RED		"\E[1m\E[31m"
#define COLOR_GREEN		"\E[1m\E[32m"
#define COLOR_YELLOW	"\E[1m\E[33m"

#define ASSERT(x) if( ! (x) ) { print_trace(); exit(1); }

#define BASE_ERROR_STR COLOR_YELLOW "%s:%d" COLOR_DEFAULT ": " COLOR_RED "%s()" COLOR_DEFAULT ": "
#define BASE_ERROR_ARGS __FILE__, __LINE__, __FUNCTION__

#define ERROR( FMT, ... ) \
	{ \
		char buf[1024]; \
		printf( BASE_ERROR_STR FMT "\n", BASE_ERROR_ARGS, ##__VA_ARGS__ ); \
		print_trace(); \
		exit(1); \
	}

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
}
#endif

#endif

