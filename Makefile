.PHONY: all clean cleanall performance performance-selinux test

CONF			 = ../configure
CONFOPTS		 = --enable-option-checking=fatal --enable-load-extension
CONFOPTS		+= --enable-static
NO_THREADSAFE		 = --enable-threadsafe=no
ENABLE_SELINUX	 	 = --enable-selinux
ENABLE_DEBUG	 	 = --enable-debug
ENABLE_STATIC_CONTEXT	 = --enable-selinux-static-context=yes
CONTEXTS		:= sesqlite_contexts

all:
	make build CONFOPTS="$(CONFOPTS) $(ENABLE_SELINUX) $(ENABLE_DEBUG)"
	cp test/sesqlite/policy/$(CONTEXTS) build

all_static_context:
	make build CONFOPTS="$(CONFOPTS) $(ENABLE_SELINUX) $(ENABLE_DEBUG) $(ENABLE_STATIC_CONTEXT)"
	cp test/sesqlite/policy/$(CONTEXTS) build

build:  configure
	mkdir -p build
	cd build; $(CONF) $(CONFOPTS); make

configure:
	autoconf -i
	
test:
	make fresh -C test/sesqlite/cunit
	make graph -C test/sesqlite/performance

performance: clean
	make build CONFOPTS="$(CONFOPTS) $(NO_THREADSAFE)"

performance-selinux: clean
	make build CONFOPTS="$(CONFOPTS) $(NO_THREADSAFE) $(ENABLE_SELINUX)"

clean:
	@- $(RM) -rf build
	@- $(RM) configure

cleanall: clean
	@ make clean -C test/sesqlite/cunit
	@ make clean -C test/sesqlite/performance
