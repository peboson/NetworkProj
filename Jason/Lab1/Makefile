EXE = count
SRC = count.c
CC = gcc
CC_OPTIONS = -g -Wall
CS5462 = .
INCLUDES = -I$(CS5462)
CFLAGS = $(CC_OPTIONS) $(INCLUDES)
LIBS = -L$(CS5462)
OBJ = $(SRC:.c=.o)

.PHONY: clean test

$(EXE): $(OBJ) 
	$(CC) $(CC_OPTIONS) -o $@ $(OBJ) $(LIBS)
clean: 
	rm -f $(EXE)
	rm -f file_occupy
	rm -f *.o
	rm -f *.out
	rm -f core
	rm -f *~
depend: Makefile $(SRC)
	makedepend $(INCLUDES) $(SRC)
.c.o:
	$(CC) $(CC_OPTIONS) $(INCLUDES) -c -o $@ $<

file_occupy: file_occupy.o
	$(CC) $(CC_OPTIONS) -o $@ $< $(LIBS)

test: $(EXE) file_occupy
test11: test
	./count example1 "Sly Fox" example11.out
test12: test
	./count example1 the example12.out
test21: test
	./count example2 tomato example21.out
test22: test
	./count example2 potato example22.out
## end of Makefile # DO NOT DELETE THIS LINE -- make depend depends on it.
