all:
	rm -rf build
	mkdir build
	bash -c "cd build; ../configure; make clean; make"
