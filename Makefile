.PHONY : clean all

clean:
	rm -rf build

build:
	mkdir -p build
	cd build; ../configure --enable-option-checking=fatal --enable-load-extension --enable-selinux; make clean; make
	
all: | clean build
