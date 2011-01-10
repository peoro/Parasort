MPICC = mpicc
MPICPP = mpic++
CFLAGS = -Wall -Wextra -Wno-unused -Werror -O2 -g

OBJECTS = ../debug.o
TESTS = sendrecv probe tsetup

all: tests

tests:  $(TESTS)

$(TESTS): $(OBJECTS)
	@echo "Compiling $@..."
	$(MPICC) $(CFLAGS) $@.c $(OBJECTS) -o $@

clean:
	rm -f *~ *.o $(TESTS)
