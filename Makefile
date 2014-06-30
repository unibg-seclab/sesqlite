.PHONY: all clean performance performance-selinux test

CONF			= ../configure
CONFOPTS		= --enable-option-checking=fatal --enable-load-extension
NO_THREADSAFE	= --enable-threadsafe=no
ENABLE_SELINUX	= --enable-selinux
ENABLE_DEBUG	= --enable-debug
SESQLITE_CONTEXT        := sesqlite_contexts

build:
	mkdir -p build
	cp test/sesqlite/policy/$(SESQLITE_CONTEXT) ./$(SESQLITE_CONTEXT)
	cd build; $(CONF) $(CONFOPTS); make

all:
	make build CONFOPTS="$(CONFOPTS) $(ENABLE_SELINUX) $(ENABLE_DEBUG)"

test:
	make fresh -C test/sesqlite/cunit

performance: clean
	make build CONFOPTS="$(CONFOPTS) $(NO_THREADSAFE)"

performance-selinux: clean
	make build CONFOPTS="$(CONFOPTS) $(NO_THREADSAFE) $(ENABLE_SELINUX)"

clean:
	@- $(RM) -rf build
	@ make clean -C test/sesqlite/cunit
	@ make clean -C test/sesqlite/performance
