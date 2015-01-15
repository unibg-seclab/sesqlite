.PHONY: all test clean fresh fresh-test

n = 10
CFLAGS = -O3
LFLAGS = -lselinux

all avctest:
	gcc $(CFLAGS) avctest.c $(LFLAGS) -o avctest

test: avctest
	@./avctest $(n)

clean:
	rm -f avctest

fresh: | clean all

fresh-test: | clean test
