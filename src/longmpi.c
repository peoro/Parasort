#include "longmpi.h"

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
extern "C" {
#endif

int _MPI_Get_count( MPI_Status *status, MPI_Datatype datatype, long *count ) {
	int c = *count;
	int ret = MPI_Get_count( status, datatype, &c );
	*count = c;
	return ret;
}

int _MPI_Recv( void *buf, long count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status ) {
	return MPI_Recv( buf, count, datatype, source, tag, comm, status );
}

int _MPI_Send( void *buf, long count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm ) {
	return MPI_Send( buf, count, datatype, dest, tag, comm );
}

int _MPI_Bcast ( void *buffer, long count, MPI_Datatype datatype, int root, MPI_Comm comm ) {
	return MPI_Bcast ( buffer, count, datatype, root, comm );
}

int _MPI_Scatter ( void *sendbuf, long sendcnt, MPI_Datatype sendtype, void *recvbuf, long recvcnt, MPI_Datatype recvtype, int root, MPI_Comm comm ) {
	return MPI_Scatter ( sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm );
}

int _MPI_Gather ( void *sendbuf, long sendcnt, MPI_Datatype sendtype, void *recvbuf, long recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm ) {
	return MPI_Gather ( sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype, root, comm );
}

int _MPI_Alltoall( void *sendbuf, long sendcount, MPI_Datatype sendtype, void *recvbuf, long recvcnt, MPI_Datatype recvtype, MPI_Comm comm ) {
	return MPI_Alltoall( sendbuf, sendcount, sendtype, recvbuf, recvcnt, recvtype, comm );
}

/* Keep C++ compilers from getting confused */
#if defined(__cplusplus)
}
#endif


