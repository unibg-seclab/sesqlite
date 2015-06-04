.PHONY: all clean install uninstall

MODULE_NAME	:= sqlite
MODULE_TE	:= $(MODULE_NAME).te
MODULE_MOD	:= $(MODULE_NAME).mod
MODULE_PP	:= $(MODULE_NAME).pp
VALIDATOR	:= ./validate_contexts
CONTEXTS	:= sesqlite_contexts

all: $(MODULE_PP)

$(MODULE_MOD):
	checkmodule -M -m $(MODULE_TE) -o $(MODULE_MOD)

$(MODULE_PP): $(MODULE_MOD)
	semodule_package -m $(MODULE_MOD) -o $(MODULE_PP)

clean:
	@- rm -f $(MODULE_MOD) $(MODULE_PP)

install: uninstall
	@ sudo semodule -v -i $(MODULE_PP)

uninstall:
	@ if sudo semodule -l | grep -q $(MODULE_NAME) ; \
	    then sudo semodule -v -r $(MODULE_NAME) ; \
	  fi

validate:
	$(VALIDATOR) -v $(CONTEXTS)

