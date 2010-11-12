MPICC = mpicc
MPICPP = mpic++
CFLAGS = -Wall -Wextra -O2 -g
ALGOCFLAGS = $(CFLAGS) -I.. -I. -L.. -fPIC -shared

INSTALLDIR = ~/.spd
DATADIR = $(INSTALLDIR)/data
LIBDIR = $(INSTALLDIR)/algorithms
ALGODIR = algorithms
ALGOS = mergesort samplesort bucketsort bitonicsort nosort
ALGOSPP = kmerge

all: clean $(ALGOS) $(ALGOSPP) main maindbg

$(ALGOS):
	@echo "Compiling "$@"..."
	$(MPICC) $(ALGOCFLAGS) -Wl,-soname,$@ -o $(ALGODIR)/lib$@.so $(ALGODIR)/$@.c

$(ALGOSPP):
	@echo "Compiling "$@"..."
	$(MPICPP) $(ALGOCFLAGS) -Wl,-soname,$@ -o $(ALGODIR)/lib$@.so $(ALGODIR)/$@.c

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
	@rm -f *~ *.o spd spddbg
