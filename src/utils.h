//computes the logarithm base 2 of a positive integer
inline unsigned int _log2 ( unsigned int n ) 
{
    unsigned int toRet = 0;
    int m = n - 1;
    while (m > 0) {
		m >>= 1;
		toRet++;
    }
    return toRet;
}

//compare two integers
inline int compare (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}
