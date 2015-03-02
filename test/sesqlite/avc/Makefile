.PHONY: all test clean fresh fresh-test

n := 1000

all: test

avctest/avctest:
	make -C avctest all

uavctest/uavctest:
	make -C uavctest all

test: avctest/avctest uavctest/uavctest
	python test.py $(n)

clean:
	make -C avctest clean
	make -C uavctest clean

distclean: clean

fresh: | clean all

fresh-test: | clean test
