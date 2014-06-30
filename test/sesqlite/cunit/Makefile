# Makefile for building sesqlite_test

.PHONY: all clean distclean fresh test

TOP				 = ../../..
NAME			:= test_sesqlite
OBJS			+= test_sesqlite1.o sqlite3.o
LDFLAGS			+= -lselinux -lpthread -ldl -lcunit

CUNIT_XML		:= cunit_testall-Results.xml		# do not change
JUNIT_XML		:= junit_testall-Results.xml
CUNIT_TO_JUNIT	:= cunit-to-junit.xsl
CONTEXTS		:= sesqlite_contexts

all: test

sqlite3.o:
	@ make all -C $(TOP)
	cp $(TOP)/build/sqlite3.o .

$(CONTEXTS):
	cp ../policy/$@ .

$(NAME): $(OBJS)
	gcc $(OBJS) -o $(NAME) $(LDFLAGS)

$(CUNIT_XML): $(CONTEXTS)
	@- ./$(NAME) -auto

$(JUNIT_XML) test: $(NAME) $(CUNIT_XML)
	@ xsltproc --novalid -o $(JUNIT_XML) $(CUNIT_TO_JUNIT) $(CUNIT_XML)

clean:
	@- $(RM) $(NAME) $(OBJS) $(CUNIT_XML) $(JUNIT_XML)

distclean: clean
	@ make clean -C $(TOP)

fresh: | distclean all
