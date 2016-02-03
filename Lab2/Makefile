EXE = ftps ftpc
SRC = ftps.c ftpc.c
CC = gcc
CC_OPTIONS = -g -Wall
CS5462 = .
INCLUDES = -I$(CS5462)
CFLAGS = $(CC_OPTIONS) $(INCLUDES)
LIBS = -L$(CS5462)
OBJ = $(SRC:.c=.o)

.PHONY: all clean test

all: ftps ftpc
	mv ftps server/ftps
	mv ftpc client/ftpc

ftps: ftps.o
	$(CC) $(CC_OPTIONS) -o $@ $< $(LIBS)
ftpc: ftpc.o
	$(CC) $(CC_OPTIONS) -o $@ $< $(LIBS)

clean: 
	rm -f ftps ftpc
	rm -f *.o
	rm -f *.out
	rm -f core
	rm -f *~
	rm -f client/ftpc
	rm -f server/*
depend: Makefile $(SRC)
	makedepend $(INCLUDES) $(SRC)
.c.o:
	$(CC) $(CC_OPTIONS) $(INCLUDES) -c -o $@ $<

test:
testsrv: test
	cd server && ./ftps 1040
testcli1: test
	cd client && ./ftpc localhost 1040 1.jpg
testcli2: test
	cd client && ./ftpc localhost 1040 2.jpg
testcli3: test
	cd client && ./ftpc localhost 1040 3.jpg
testcli4: test
	cd client && ./ftpc localhost 1040 4.jpg
## end of Makefile # DO NOT DELETE THIS LINE -- make depend depends on it.
