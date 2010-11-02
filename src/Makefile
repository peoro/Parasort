MPICC = mpicc
CFLAGS = -Wall 
LIBDIR = ~/.spd/algorithms/
ALGO_MERGESORT = mergesort
ALGO_SAMPLESORT = samplesort

all: clean mergesort samplesort main 
	
mergesort:
	@echo "Compiling lib-mergesort..."
	@$(MPICC) -g -I.. -L.. -L/usr/lib/openmpi -lmpi -shared -O2 -Wl,-soname,$(ALGO_MERGESORT) -o lib$(ALGO_MERGESORT).so $(ALGO_MERGESORT).c && mv lib$(ALGO_MERGESORT).so $(LIBDIR)

samplesort:
	@echo "Compiling lib-samplesort..."
	@$(MPICC) -fPIC -O2 -shared -Wl,-soname,lib$(ALGO_SAMPLESORT).so -o lib$(ALGO_SAMPLESORT).so $(ALGO_SAMPLESORT).c && mv lib$(ALGO_SAMPLESORT).so $(LIBDIR)

main: 
	@echo "Compiling main..."
	@$(MPICC) $(CFLAGS) main.c -o main
	
clean:
	clear
	@rm -f *~ *.o main 
