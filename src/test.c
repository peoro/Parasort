#include <stdio.h>

#include "sorting.h"

int main ( int argc, char *argv[] ) 
{
    MPI_Init (&argc, &argv);
    TestInfo ti;
    ti.M = 97;
    printf ( "id = %i, local_M = %li\n", GET_ID(&ti), GET_LOCAL_M(&ti) );
    return 0;
}