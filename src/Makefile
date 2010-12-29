MPICC = mpicc
MPICPP = mpic++
CFLAGS = -Wall -Wextra -Wno-unused -O2 -g
ALGOCFLAGS = $(CFLAGS) -I.. -I. -L.. -fPIC

INSTALLDIR = $(HOME)/.spd
DATADIR = $(INSTALLDIR)/data
LIBDIR = $(INSTALLDIR)/algorithms
ALGODIR = algorithms
ALGOS = mergesort samplesort bucketsort bitonicsort # lbmergesort nosort sequential
ALGOSPP = kmerge # lbkmergesort
OBJECTS = communication.o sorting.o

all: $(ALGOS) $(ALGOSPP) main

$(ALGOS): $(OBJECTS)
	@echo "Compiling "$@"..."
	$(MPICC) -lm $(ALGOCFLAGS) -shared -Wl,-soname,$@ -o $(ALGODIR)/lib$@.so $(ALGODIR)/$@.c $(OBJECTS)

$(ALGOSPP): $(OBJECTS)
	@echo "Compiling "$@"..."
	$(MPICPP) $(ALGOCFLAGS) -shared -Wl,-soname,$@ -o $(ALGODIR)/lib$@.so $(ALGODIR)/$@.c $(OBJECTS)

$(OBJECTS):
	$(MPICC) $(ALGOCFLAGS) -c $$( echo $@ | sed -rs "s/\.o/.c/" )

install:
	mkdir -p $(LIBDIR) $(DATADIR)
	cp $(ALGODIR)/lib*.so $(LIBDIR)

main: spd spddbg spdlog

spd: spd.c  $(OBJECTS)
	@echo "Compiling spd..."
	$(MPICC) $(CFLAGS) -ldl -rdynamic spd.c $(OBJECTS) -o spd

spddbg: spd.c $(OBJECTS)
	@echo "Compiling spddbg (debug mode)..."
	$(MPICC) $(CFLAGS) -ldl -rdynamic spd.c $(OBJECTS) -DDEBUG -o spddbg

spdlog: spd.c
	@echo "Compiling spdlog (profiling mode)..."
	$(MPICC) $(CFLAGS) -ldl -rdynamic spd.c $(OBJECTS) -mpe=mpilog -o spdlog

clean:
	rm -f *~ *.o *.clog2 spd spddbg spdlog
