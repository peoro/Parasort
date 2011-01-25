MPICC = mpicc
MPICPP = mpic++
CFLAGS = -D_FILE_OFFSET_BITS=64 -Wall -Wextra -Wno-unused -Werror -O2 -g
ALGOCFLAGS = $(CFLAGS) -I.. -I. -L.. -fPIC

INSTALLDIR = $(HOME)/.spd
DATADIR = $(INSTALLDIR)/data
LIBDIR = $(INSTALLDIR)/algorithms
ALGODIR = algorithms
ALGOS = sequential mergesort bucketsort samplesort bitonicsort lbmergesort sequential1 # nosort quicksort
ALGOSPP = kmerge lbkmergesort
OBJECTS = dal.o debug.o sorting.o common.o

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

tests: force_look
	make -C tests -f Makefile clean tests # wtf, why are we using GNU Make? CMake/SCons FTW!

clean:
	rm -f *~ *.o *.clog2 spd spddbg spdlog

force_look:
	true
