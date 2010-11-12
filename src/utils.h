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
