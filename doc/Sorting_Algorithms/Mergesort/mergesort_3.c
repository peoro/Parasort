/*
! This program implements a parallel merge sort.
! See the class notes for an explanation of merge works.
! We start with a sorted list on each processor.
! Note that the Merge could be done with half the sends/recs
! if we used a MPI_PROBE to check message size before the
! MPI_RECV
*/
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

/* module global */
int numnodes,myid,mpi_err;
#define mpi_root 0
/* end module  */

void init_it(int  *argc, char ***argv);
void merge2(int d1[],int nin,int d2[],int min,int out[]);
int MY_MERGE(int input[], int n,
              int output[],
              int root,int THETAG,MPI_Comm THECOM,MPI_Datatype MPI_FIT);


int main(int argc,char *argv[]){
	int *input,*output;
	int the_size,i,j,m,total_size,root,THETAG;
	init_it(&argc,&argv);
	THETAG=123;
	root=0;
	the_size=4;
	total_size=the_size*numnodes;
	input=(int*)malloc(the_size*sizeof(int));
	for( i=0;i<the_size;i++)
	   input[i]=myid+(i)*numnodes;
	if(myid == root)output=(int*)malloc(total_size*sizeof(int));
	MY_MERGE(input,the_size,output,root,THETAG,MPI_COMM_WORLD,MPI_INT);
	if(myid == root) {
		printf("output ");
		for(i=0;i<total_size;i++)
			printf("%d ",output[i]);
		printf("\n");
	}
	mpi_err = MPI_Finalize();
}

void init_it(int  *argc, char ***argv) {
	mpi_err = MPI_Init(argc,argv);
    mpi_err = MPI_Comm_size( MPI_COMM_WORLD, &numnodes );
    mpi_err = MPI_Comm_rank(MPI_COMM_WORLD, &myid);
}

void merge2(int d1[],int nin,int d2[],int min,int out[]) {
  int i,j,k,m,n;
  i=0;
  j=0;
  m=min-1;
  n=nin-1;
	for(k=0;k<n+m+2;k++){
		if(i > n ){
				out[k]=d2[j];/*printf("at 1  %d %d %d\n",k,j,out[k]);*/
				j=j+1;
			}
    		else if(j > m){
      			out[k]=d1[i];/*printf("at 2  %d %d %d\n",k,i,out[k]);*/
      			i=i+1;
    		}
    	else{
     		 if(d1[i] < d2[j]){
       			 out[k]=d1[i];/*printf("at 3  %d %d %d\n",k,i,out[k]);*/
        		i=i+1;
			}
     		else {
    	    	out[k]=d2[j];/*printf("at 4  %d %d %d\n",k,j,out[k]);*/
        		j=j+1;
     		}
		}
   	}
}

int MY_MERGE(int input[], int n,
              int output[],
              int root,int THETAG,MPI_Comm THECOM,MPI_Datatype MPI_FIT) {
    int ntag;
    int *rec,*temp,*temp2;
    int *data;
    int p,active,ierr,count,i,ijk,m;
    int ic;
    MPI_Status status;
    ntag=987;
    active=1;
    p=numnodes;
    rec=NULL;temp=NULL;temp2=NULL,data=NULL;
    while (2*active < p) {
    	active=active*2;
    }
    if(myid >= active){
    /* send(input,myid-active) */
    	ierr = MPI_Send(&n,1,MPI_INT,myid-active,ntag,THECOM);
    	ierr = MPI_Send(input,n,MPI_FIT,myid-active,THETAG,THECOM);
    	return(0);
    }
    data = (int*)malloc(n*sizeof(int));
    for(ic=0;ic<n;ic++)
    	data[ic]=input[ic];
    m=n;
    if(myid + active  < p){
    /* rec(new_data,myid+active) */
    	ierr =  MPI_Recv(&count,1,MPI_INT,myid+active,ntag,THECOM,&status);
    	free(rec);
    	rec=(int*)malloc(count*sizeof(int));
    	ierr =  MPI_Recv(rec,count,MPI_FIT,myid+active,THETAG,THECOM,&status);
    
    /* data=data+new_data */
    	free(temp);
    	temp=(int*)malloc((n+count)*sizeof(int));
    	merge2(input,n,rec,count,temp);
    	free(data);
    	data=(int*)malloc((n+count)*sizeof(int));
    	for(ic=0;ic<(n+count);ic++)
    		data[ic]=temp[ic];
    	m=n+count;
    }
    
    while (active > 1) {
    	active=active/2;
    	if(myid >= active) {
    /* send(data,myid-active) */
    		ierr = MPI_Send(&m,1,MPI_INT,myid-active,ntag,THECOM);
    		ierr = MPI_Send(data,m,MPI_FIT,myid-active,THETAG,THECOM);
    		free(rec);free(temp);free(temp2),free(data);
    		return(0);
    	}
    	else {
    /* recv(new_data,myid+active) */
    		ierr = MPI_Recv(&count,1,MPI_INT,myid+active,ntag,THECOM,&status);
    		free(rec);
    		rec=(int*)malloc(count*sizeof(int));
    		ierr = MPI_Recv(rec,count,MPI_FIT,myid+active,THETAG,THECOM,&status);
    /* data=data+new_data */
    		free(temp2);
    		temp2=(int*)malloc((m+count)*sizeof(int));
    		merge2(data,m,rec,count,temp2);
    		free(data);
    		data=(int*)malloc((m+count)*sizeof(int));
    		for(ic=0;ic<(m+count);ic++)
    			data[ic]=temp2[ic];
    		m=(m)+count;    
    	}
    }
    for(ic=0;ic<m;ic++)
    	output[ic]=data[ic];
    free(rec);free(temp);free(temp2),free(data);
    return(m);	
}
