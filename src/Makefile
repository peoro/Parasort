MPICC = mpicc
MPICPP = mpic++
CFLAGS = -Wall -Wextra -Wno-unused -O2 -g
ALGOCFLAGS = $(CFLAGS) -I.. -I. -L.. -fPIC

INSTALLDIR = ~/.spd
DATADIR = $(INSTALLDIR)/data
LIBDIR = $(INSTALLDIR)/algorithms
ALGODIR = algorithms
ALGOS = mergesort quicksort samplesort bucketsort bitonicsort nosort
ALGOSPP = kmerge
OBJECTS = longmpi.o

all: $(ALGOS) $(ALGOSPP) main

$(ALGOS): longmpi.o
	@echo "Compiling "$@"..."
	$(MPICC) -lm $(ALGOCFLAGS) -shared -Wl,-soname,$@ -o $(ALGODIR)/lib$@.so $(ALGODIR)/$@.c $(OBJECTS)

$(ALGOSPP): longmpi.o
	@echo "Compiling "$@"..."
	$(MPICPP) $(ALGOCFLAGS) -shared -Wl,-soname,$@ -o $(ALGODIR)/lib$@.so $(ALGODIR)/$@.c $(OBJECTS)

longmpi.o:
	$(MPICC) $(ALGOCFLAGS) -c longmpi.c

install:
	mkdir -p $(LIBDIR) $(DATADIR)
	cp $(ALGODIR)/lib*.so $(LIBDIR)

main: spd spddbg

spd: spd.c
	@echo "Compiling spd..."
	$(MPICC) $(CFLAGS) -ldl spd.c -o spd

spddbg: spd.c
	@echo "Compiling spddbg (debug mode)..."
	$(MPICC) $(CFLAGS) -ldl spd.c -DDEBUG -o spddbg

clean:
	rm -f *~ *.o spd spddbg
