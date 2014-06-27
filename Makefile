.PHONY: all clean performance performance-selinux

CONF            = ../configure
CONFOPTS        = --enable-option-checking=fatal --enable-load-extension
NO_THREADSAFE   = --enable-threadsafe=no
ENABLE_SELINUX  = --enable-selinux
ENABLE_DEBUG    = --enable-debug

build:
	mkdir -p build
	cd build; $(CONF) $(CONFOPTS); make

all:
	make build CONFOPTS="$(CONFOPTS) $(ENABLE_SELINUX) $(ENABLE_DEBUG)"

performance: clean
	make build CONFOPTS="$(CONFOPTS) $(NO_THREADSAFE)"

performance-selinux: clean
	make build CONFOPTS="$(CONFOPTS) $(NO_THREADSAFE) $(ENABLE_SELINUX)"

clean:
	@- $(RM) -rf build
