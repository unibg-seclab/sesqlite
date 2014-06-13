.PHONY : clean all

clean:
	rm -rf build

all build:
	mkdir -p build
	cd build; ../configure --enable-option-checking=fatal --enable-load-extension --enable-selinux --enable-debug; make
