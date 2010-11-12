//efficiently computes the logarithm base 2 of a positive integer
inline int _log2 ( unsigned int n ) 
{
    unsigned int toRet = 0;
    int m = n - 1;
    while (m > 0) {
		m >>= 1;
		toRet++;
    }
    return toRet;
}
inline int _pow2( int n ) {
	int i, r = 1;
	for( i = 0; i < n; ++ i ) {
		r *= 2;
	}
	return r;
}

//computes the logarithm base k of a positive integer
inline int _logk ( unsigned int n, unsigned int k ) 
{
    return _log2 ( n ) / _log2 ( k );
}


//compare two integers
inline int compare (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

#include <mpi.h>
int _MPI_Get_count( MPI_Status *status, MPI_Datatype datatype, long *count ) {
	int c = *count;
	int ret = MPI_Get_count( status, datatype, &c );
	*count = c;
	return ret;
}
#define _MPI_Recv(a,b,c,d,e,f,g) MPI_Recv(a,b,c,d,e,f,g)
#define _MPI_Send(a,b,c,d,e,f) MPI_Send(a,b,c,d,e,f)
