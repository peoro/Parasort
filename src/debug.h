

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include<assert.h>

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
extern "C" {
#endif


int SPD_getPid( );
const char *SPD_makeColor( int b, int fg, int bg );

void print_trace( void ); // TODO: pure debugging, move in debug.c ...

#define SPD_ESCAPE_COLOR(c)		"\E[" #c "]"
#define SPD_COLOR_CODE(b,bg,fg)	SPD_ESCAPE_COLOR(b) SPD_ESCAPE_COLOR(bg) SPD_ESCAPE_COLOR(fg)

#define SPD_COLOR_DEFAULT	"\E[0m"
#define SPD_COLOR_RED		"\E[1m\E[31m"
#define SPD_COLOR_GREEN		"\E[1m\E[32m"
#define SPD_COLOR_YELLOW	"\E[1m\E[33m"

#define SPD_BASE_ERROR_STR		"[%s%d" SPD_COLOR_DEFAULT "] " SPD_COLOR_YELLOW "%s:%d" SPD_COLOR_DEFAULT ": " SPD_COLOR_RED "%s()" SPD_COLOR_DEFAULT ": "
#define SPD_BASE_ERROR_ARGS		SPD_makeColor(1, SPD_getPid()+1, SPD_getPid()), SPD_getPid(), __FILE__, __LINE__, __FUNCTION__

#define SPD_ERROR( fmt, ... ) \
	{ \
		char buf[1024]; \
		printf( SPD_BASE_ERROR_STR fmt "\n", SPD_BASE_ERROR_ARGS, ##__VA_ARGS__ ); \
		print_trace(); \
		exit(1); \
	}

#define SPD_DEBUG( fmt, ... ) \
	{ \
		char buf[1024]; \
		printf( SPD_BASE_ERROR_STR fmt "\n", SPD_BASE_ERROR_ARGS, ##__VA_ARGS__ ); \
	}

#define SPD_ASSERT(cond, fmt, ... ) \
	if( ! (cond) ) { \
		SPD_ERROR( fmt "\n  (`" SPD_COLOR_GREEN "%s" SPD_COLOR_DEFAULT "` failed)", ##__VA_ARGS__, #cond ); \
	}

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
}
#endif

#endif

