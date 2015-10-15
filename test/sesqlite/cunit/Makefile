# Makefile for building sesqlite_test

.PHONY: all clean distclean fresh test

TOP				 = ../../..
TEST_SCHEMA			:= test_schema_level
TEST_TUPLE			:= test_tuple_level
OBJS			+= sqlite3.o
LDFLAGS			+= -lselinux -lpthread -ldl -lcunit

CUNIT_XML		:= cunit_testall-Results.xml		# do not change
JUNIT_XML		:= junit_testall-Results.xml
CUNIT_TO_JUNIT	:= cunit-to-junit.xsl
CONTEXTS		:= sesqlite_contexts

all: $(TEST_SCHEMA) \
	$(TEST_TUPLE) \
	$(CONTEXTS)
	@ ./$(TEST_SCHEMA) -auto
	@ ./$(TEST_TUPLE) -auto

sqlite3.o:
	@ make all -C $(TOP)
	cp $(TOP)/build/sqlite3.o .
	cp $(TOP)/build/sqlite3.h .

$(TEST_SCHEMA): $(OBJS) $(TEST_SCHEMA).o
	gcc $(CFLAGS) $(OBJS) $(TEST_SCHEMA).o -o $(TEST_SCHEMA) $(LDFLAGS)

$(TEST_TUPLE): $(OBJS) $(TEST_TUPLE).o
	gcc $(CFLAGS) $(OBJS) $(TEST_TUPLE).o -o $(TEST_TUPLE) $(LDFLAGS)

$(CONTEXTS):
	cp $(TOP)/test/sesqlite/policy/$(CONTEXTS) .

$(CUNIT_XML): $(CONTEXTS)
	@- ./$(TEST_SCHEMA) -auto

$(JUNIT_XML) test: $(TEST_SCHEMA) $(CUNIT_XML)
	@ xsltproc --novalid -o $(JUNIT_XML) $(CUNIT_TO_JUNIT) $(CUNIT_XML)

run-schema: clean-test \
	$(CONTEXTS)
	@ ./$(TEST_SCHEMA) -auto

run-tuple: clean-test \
	$(CONTEXTS)
	@ ./$(TEST_TUPLE) -auto

clean: clean-test
	@- $(RM) $(OBJS)

clean-test: 
	@- $(RM) $(TEST_SCHEMA) $(TEST_SCHEMA).o $(TEST_TUPLE) $(TEST_TUPLE).o $(CUNIT_XML) $(JUNIT_XML) $(CONTEXTS)

distclean: clean
	@ make clean -C $(TOP)

fresh: | distclean all
