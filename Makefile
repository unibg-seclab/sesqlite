.PHONY: all clean performance performance-selinux test

CONF			 = ../configure
CONFOPTS		 = --enable-option-checking=fatal --enable-load-extension
NO_THREADSAFE	 = --enable-threadsafe=no
ENABLE_SELINUX	 = --enable-selinux
ENABLE_DEBUG	 = --enable-debug
CONTEXTS		:= sesqlite_contexts

build:
	mkdir -p build
	cp test/sesqlite/policy/$(CONTEXTS) build
	cd build; $(CONF) $(CONFOPTS); make

all:
	make build CONFOPTS="$(CONFOPTS) $(ENABLE_SELINUX) $(ENABLE_DEBUG)"

test:
	make fresh -C test/sesqlite/cunit
	make graph -C test/sesqlite/performance

performance: cleanbuild
	make build CONFOPTS="$(CONFOPTS) $(NO_THREADSAFE)"

performance-selinux: cleanbuild
	make build CONFOPTS="$(CONFOPTS) $(NO_THREADSAFE) $(ENABLE_SELINUX)"

cleanbuild:
	@- $(RM) -rf build

clean: cleanbuild
	@ make clean -C test/sesqlite/cunit
	@ make clean -C test/sesqlite/performance
