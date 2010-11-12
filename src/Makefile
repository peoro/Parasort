MPICC = mpicc
MPICPP = mpic++
CFLAGS = -Wall -Wextra -Wno-unused -O2 -g
ALGOCFLAGS = $(CFLAGS) -I.. -I. -L.. -fPIC -shared

INSTALLDIR = ~/.spd
DATADIR = $(INSTALLDIR)/data
LIBDIR = $(INSTALLDIR)/algorithms
ALGODIR = algorithms
ALGOS = mergesort quicksort samplesort bucketsort bitonicsort nosort
ALGOSPP = kmerge
OBJECTS = longmpi.o

all: $(ALGOS) $(ALGOSPP) main maindbg 

$(ALGOS): longmpi.o
	@echo "Compiling "$@"..."
	$(MPICC) $(ALGOCFLAGS) -Wl,-soname,$@ -o $(OBJECTS) $(ALGODIR)/lib$@.so $(ALGODIR)/$@.c

$(ALGOSPP): longmpi.o
	@echo "Compiling "$@"..."
	$(MPICPP) $(ALGOCFLAGS) -Wl,-soname,$@ -o $(OBJECTS) $(ALGODIR)/lib$@.so $(ALGODIR)/$@.c

longmpi.o:
	$(MPICC) -c longmpi.c

install:
	mkdir -p $(LIBDIR) $(DATADIR)
	cp $(ALGODIR)/lib*.so $(LIBDIR)

main:
	@echo "Compiling main..."
	$(MPICC) $(CFLAGS) -ldl main.c -o spd

maindbg:
	@echo "Compiling main (debug mode)..."
	$(MPICC) $(CFLAGS) -ldl main.c -DDEBUG -o spddbg

clean:
	rm -f *~ *.o spd spddbg
