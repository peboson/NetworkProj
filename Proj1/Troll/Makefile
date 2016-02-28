# Makefile for Network Troll to work under Solaris 2

#CFLAGS = -I/usr/ucbinclude
CFLAGS = 
#CFLAGS = -g
#CIFLAGS = -l -sDistrib
RCSDIFFFLAGS = -c
CC = gcc 
#LIB = -lm -lsocket -lc -lnsl -L/usr/ucblib -R/usr/ucblib -lucb
#LIB = -lm -lsocket -lnsl 
LIB = -lm -lc

ALL = fromtroll totroll
SRC = Makefile fromtroll.c totroll.c troll.h

all: $(ALL)

clean: 
	rm -f *.o $(ALL)

troll: troll.o
	$(CC) $(CFLAGS) -o troll troll.o $(LIB)

fromtroll: fromtroll.o
	$(CC) $(CFLAGS) -o fromtroll fromtroll.o $(LIB)

totroll: totroll.o
	$(CC) $(CFLAGS) -o totroll totroll.o $(LIB)

