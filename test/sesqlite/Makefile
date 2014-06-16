#
# Makefile for building sesqlite_test
#
LIBS= -lsqlite3 -lselinux -lpthread -ldl -lcunit 
TARGETS = tests

tests: 

all:  $(TARGETS)

tests:  test_sesqlite1.c
	$(CC) -o test_sesqlite1 test_sesqlite1.c $(LIBS) 

clean:
	rm -f test_sesqlite1 


