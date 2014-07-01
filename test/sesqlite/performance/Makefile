.PHONY: all clean clean_test_db graph test test_se

n				 = 2
TOP				 = ../../..
NAME			:= speedtest8
NAME_SE			:= $(NAME)_se
TEST_DB			:= test.db
TEST_SQL		:= test.sql
CONTEXTS		:= sesqlite_contexts
SQLITE_OBJ		:= sqlite3.o
SE_SQLITE_OBJ	:= sesqlite3.o

SRCS			:= $(wildcard *.c)
OBJS			:= $(SRCS:.c=.o)
INCLUDES		:= -I$(TOP)/src
LDFLAGS			+= -ldl


all: $(NAME) $(NAME_SE) $(TEST_SQL)

test: $(NAME) $(TEST_SQL) clean_test_db $(CONTEXTS)
	@ ./$(NAME) -quiet $(TEST_DB) $(TEST_SQL)

test_se: $(NAME_SE) $(TEST_SQL) clean_test_db $(CONTEXTS)
	@ ./$(NAME_SE) -quiet $(TEST_DB) $(TEST_SQL)

$(NAME): $(OBJS) $(SQLITE_OBJ)
	gcc $(OBJS) $(SQLITE_OBJ) -o $(NAME) $(INCLUDES) $(LDFLAGS)

$(NAME_SE): $(OBJS) $(SE_SQLITE_OBJ)
	gcc $(OBJS) $(SE_SQLITE_OBJ) -o $(NAME_SE) $(INCLUDES) $(LDFLAGS) -lselinux

$(CONTEXTS):
	cp $(TOP)/test/sesqlite/policy/$@ .

$(SE_SQLITE_OBJ):
	@ make performance-selinux -C $(TOP)
	cp $(TOP)/build/sqlite3.o ./$(SE_SQLITE_OBJ)

$(SQLITE_OBJ):
	@ make performance -C $(TOP)
	cp $(TOP)/build/sqlite3.o ./$(SQLITE_OBJ)

$(TEST_SQL):
	@ echo "[Generating $@]"
	tclsh $(TOP)/tool/mkspeedsql.tcl > $(TEST_SQL)

clean: clean_test_db
	@- $(RM) $(NAME) $(NAME_SE)
	@- $(RM) $(OBJS) $(SE_SQLITE_OBJ) $(SQLITE_OBJ)
	@- $(RM) -rf results

clean_test_db:
	@- $(RM) $(TEST_DB)*

graph: | all results/base.out results/sesqlite.out
	@ python makegraph.py

results:
	@ mkdir results

results/base.out: results
	@ for i in `seq 1 $(n)` ; \
	    do echo "testing base ($$i) ..." ; \
	    make test $>> $@ ; \
	done

results/sesqlite.out: results
	@ for i in `seq 1 $(n)` ;\
	    do echo "testing sesqlite ($$i) ..." ; \
	    make test_se $>> $@ ; \
	done
