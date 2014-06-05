all:
	rm -rf build
	mkdir build
	cd build; ../configure; make clean; make

clean:
	rm -rf build
	