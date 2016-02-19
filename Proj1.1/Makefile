EXE = ftps ftpc
SRC = ftps.c ftpc.c tcpudp.c
CC = gcc
CC_OPTIONS = -g -Wall
CS5462 = .
INCLUDES = -I$(CS5462)
CFLAGS = $(CC_OPTIONS) $(INCLUDES)
LIBS = -L$(CS5462)
OBJ = $(SRC:.c=.o)

.PHONY: all clean test

all: ftps ftpc tcpd timer driver
	mkdir -p server
	mv ftps server/
	cp tcpd server/
	mkdir -p client
	mv ftpc client/
	mv tcpd client/
	mv timer client/
	mv driver client/

ftps: ftps.o tcpudp.o
	$(CC) $(CC_OPTIONS) -o $@ $^ $(LIBS)
ftpc: ftpc.o tcpudp.o
	$(CC) $(CC_OPTIONS) -o $@ $^ $(LIBS)
tcpd: tcpd.o tcpudp.o crc32.o checksum.o
	$(CC) $(CC_OPTIONS) -o $@ $^ $(LIBS)
timer: timer.o
	$(CC) $(CC_OPTIONS) -o $@ $^ $(LIBS)
driver: driver.o
	$(CC) $(CC_OPTIONS) -o $@ $^ $(LIBS)

clean: 
	rm -f ftps ftpc tcpd timer driver
	rm -f *.o
	rm -f *.out
	rm -f core
	rm -f *~
	rm -f client/ftpc client/tcpd client/timer client/driver
	rm -f server/*

depend: Makefile $(SRC)
	makedepend $(INCLUDES) $(SRC)
	
%.o: %.c %.h stdafx.h
	$(CC) $(CFLAGS) -c -o $@ $<

test:
ttroll: test
	cd Troll && ./troll -x0 8081
tsrvtcpd: test
	cd server && ./tcpd server
tsrvftps: test
	cd server && ./ftps 8000
tclitcpd: test
	cd client && ./tcpd client
tcli1: test
	cd client && ./ftpc beta 8000 1.jpg
tcli2: test
	cd client && ./ftpc beta 8000 2.jpg
tcli3: test
	cd client && ./ftpc beta 8000 3.jpg
tcli4: test
	cd client && ./ftpc beta 8000 4.jpg
## end of Makefile # DO NOT DELETE THIS LINE -- make depend depends on it.
