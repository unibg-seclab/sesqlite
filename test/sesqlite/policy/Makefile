.PHONY: all clean install uninstall

VALIDATOR	:= ./validate_contexts
CONTEXTS	:= sesqlite_contexts
TE			:= $(wildcard *.te)
MOD			:= $(TE:.te=.mod)
PP			:= $(TE:.te=.pp)

all: $(PP)

%.mod: %.te
	checkmodule -M -m $< -o $@

%.pp: %.mod
	semodule_package -m $< -o $@

clean:
	@ rm -f $(MOD) $(PP)

install: uninstall
	@ $(foreach p, $(shell ls *.pp), sudo semodule -v -i $(p); )
	
uninstall:
	@ $(foreach m, $(basename $(shell ls *.pp)), if sudo semodule -l | grep -q "^$(m)\b"; then sudo semodule -v -r $(m); fi; )

validate:
	$(VALIDATOR) -v $(CONTEXTS)

