.PHONY: all clean cleanall test

build:
	mkdir -p build
	cd build; ../configure --enable-option-checking=fatal --enable-load-extension --enable-threadsafe=no
	make -C build

all: build

test:
	make graph -C test/sesqlite/performance

clean:
	@- $(RM) -rf build

cleanall: clean
	@ make clean -C test/sesqlite/performance
