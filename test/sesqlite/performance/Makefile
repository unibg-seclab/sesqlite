.PHONY: all clean clean_test_db graph test test_se

n					= 2
TOP					= ../../..
NAME				:= speedtest8
NAME_TUPLE			:= speedtest8_tuple
NAME_SE				:= $(NAME)_se
NAME_TUPLE_SE		:= $(NAME)_tuple_se
TEST_DB				:= test.db
TEST_SQL			:= test.sql
TEST_TUPLE_SQL		:= test_tuple.sql
TEST_TUPLE_SE_SQL	:= test_tuple_se.sql

CONTEXTS		:= sesqlite_contexts
SQLITE_OBJ		:= sqlite3.o
SE_SQLITE_OBJ	:= sesqlite3.o

SRCS			:= $(wildcard *.c)
OBJS			:= $(SRCS:.c=.o)
INCLUDES		:= -I$(TOP)/src
LDFLAGS			+= -ldl

BASE_OUT		 = results/base.out
SE_SQLITE_OUT	 = results/sesqlite.out
BASE_TUPLE_OUT		 = results/base_tuple.out
SE_SQLITE_TUPLE_OUT	 = results/sesqlite_tuple.out

all: $(NAME) $(NAME_SE) $(TEST_SQL) $(TEST_TUPLE_SQL) $(TEST_TUPLE_SE_SQL)

test: $(NAME) $(TEST_SQL) clean_test_db $(CONTEXTS)
	@ ./$(NAME) -quiet $(TEST_DB) $(TEST_SQL)

test_se: $(NAME_SE) $(TEST_SQL) clean_test_db $(CONTEXTS)
	@ ./$(NAME_SE) -quiet $(TEST_DB) $(TEST_SQL)

test_tuple: $(NAME_TUPLE) $(TEST_TUPLE_SQL) clean_test_db $(CONTEXTS)
	@ ./$(NAME_TUPLE) -quiet $(TEST_DB) $(TEST_TUPLE_SQL)
	
test_tuple_se: $(NAME_TUPLE_SE) $(TEST_TUPLE_SE_SQL) clean_test_db $(CONTEXTS)
	@ ./$(NAME_TUPLE_SE) -quiet $(TEST_DB) $(TEST_TUPLE_SE_SQL)

$(NAME): $(OBJS) $(SQLITE_OBJ)
	gcc $(OBJS) $(SQLITE_OBJ) -o $(NAME) $(INCLUDES) $(LDFLAGS)

$(NAME_SE): $(OBJS) $(SE_SQLITE_OBJ)
	gcc $(OBJS) $(SE_SQLITE_OBJ) -o $(NAME_SE) $(INCLUDES) $(LDFLAGS) -lselinux

$(NAME_TUPLE): $(OBJS) $(SQLITE_OBJ)
	gcc $(OBJS) $(SQLITE_OBJ) -o $(NAME_TUPLE) $(INCLUDES) $(LDFLAGS)
	
$(NAME_TUPLE_SE): $(OBJS) $(SE_SQLITE_OBJ)
	gcc $(OBJS) $(SE_SQLITE_OBJ) -o $(NAME_TUPLE_SE) $(INCLUDES) $(LDFLAGS) -lselinux

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

$(TEST_TUPLE_SQL):
	@ echo "[Generating $@]"
	tclsh $(TOP)/tool/mkspeedsql_tuple.tcl > $(TEST_TUPLE_SQL)

$(TEST_TUPLE_SE_SQL):
	@ echo "[Generating $@]"
	tclsh $(TOP)/tool/mkspeedsql_tuple_se.tcl > $(TEST_TUPLE_SE_SQL)

clean: clean_test_db
	@- $(RM) $(NAME) $(NAME_SE) $(NAME_TUPLE) $(NAME_TUPLE_SE)
	@- $(RM) $(OBJS) $(SE_SQLITE_OBJ) $(SQLITE_OBJ) $(CONTEXTS)
	@- $(RM) -rf results

clean_test_db:
	@- $(RM) $(TEST_DB)*

graph: $(BASE_OUT) $(SE_SQLITE_OUT) $(BASE_TUPLE_OUT) $(SE_SQLITE_TUPLE_OUT)
	@- python makegraph.py

results:
	@ mkdir results

$(BASE_OUT): results $(TEST_SQL) $(NAME)
	@ for i in `seq 1 $(n)` ; \
	    do echo "testing base ($$i) ..." ; \
	    make test >> $@ ; \
	done

$(SE_SQLITE_OUT): results $(TEST_SQL) $(NAME_SE)
	@ for i in `seq 1 $(n)` ;\
	    do echo "testing sesqlite ($$i) ..." ; \
	    make test_se >> $@ ; \
	done
	
$(BASE_TUPLE_OUT): results $(TEST_TUPLE_SQL) $(NAME_TUPLE)
	@ for i in `seq 1 $(n)` ; \
	    do echo "testing base tuple ($$i) ..." ; \
	    make test_tuple >> $@ ; \
	done

$(SE_SQLITE_TUPLE_OUT): results $(TEST_TUPLE_SE_SQL) $(NAME_TUPLE_SE)
	@ for i in `seq 1 $(n)` ;\
	    do echo "testing sesqlite tuple ($$i) ..." ; \
	    make test_tuple_se >> $@ ; \
	done
