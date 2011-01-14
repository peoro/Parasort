MPICC = mpicc
MPICPP = mpic++
CFLAGS = -Wall -Wextra -Wno-unused -Werror -g

OBJECTS = ../debug.o
TESTS = sendrecv probe tsetup

DALOBJECTS = $(OBJECTS) ../dal.o
DALTESTS = dalsendrecv

all: tests

tests: $(TESTS) $(DALTESTS)

$(TESTS): $(OBJECTS)
	@echo "Compiling $@..."
	$(MPICC) $(CFLAGS) $@.c $(OBJECTS) -o $@

$(DALTESTS): $(OBJECTS) $(DALOBJECTS)
	@echo "Compiling $@..."
	$(MPICC) $(CFLAGS) $@.c $(DALOBJECTS) -o $@

clean:
	rm -f *~ *.o $(TESTS) $(DALTESTS)
