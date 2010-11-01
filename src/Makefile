MPICC = mpicc
CFLAGS = -Wall 

all: clean mergesort
	
mergesort:
	$(MPICC) $(CFLAGS) -c mergesort.c 

clean:
	clear
	@rm -f *~ *.o 
