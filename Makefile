.PHONY : clean all

clean:
	rm -rf build

build:
	mkdir -p build
	cd build; ../configure --enable-load-extension; make clean; make
	
all: | clean build
