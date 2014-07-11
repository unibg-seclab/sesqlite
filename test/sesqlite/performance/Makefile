.PHONY: all clean clean_temp clean_test_db graph test test_se

n					= 2
TOP					= ../../..
NAME				:= speedtest8
NAME_SE				:= $(NAME)_se
TEST_DB				:= test.db
TEST_SQL			:= test.sql
TEST_TUPLE_SQL		:= test_tuple.sql
TEST_TUPLE_SE_SQL	:= test_tuple_se.sql

CONTEXTS			:= sesqlite_contexts
SQLITE_OBJ			:= sqlite3.o
SE_SQLITE_OBJ		:= sesqlite3.o

SRCS				:= $(wildcard *.c)
OBJS				:= $(SRCS:.c=.o)
INCLUDES			:= -I$(TOP)/src
LDFLAGS				+= -ldl

BASE_OUT			 = results/base.out
SE_SQLITE_OUT		 = results/sesqlite.out
BASE_TUPLE_OUT		 = results/base_tuple.out
SE_SQLITE_TUPLE_OUT	 = results/sesqlite_tuple.out


all: $(NAME) $(NAME_SE) $(TEST_SQL) $(TEST_TUPLE_SQL) $(TEST_TUPLE_SE_SQL)

test: clean_test_db $(TEST_SQL) $(NAME)
	@ ./$(NAME) -quiet $(TEST_DB) $(TEST_SQL)

test_se: clean_test_db $(CONTEXTS) $(TEST_SQL) $(NAME_SE)
	@ ./$(NAME_SE) -quiet $(TEST_DB) $(TEST_SQL)

test_tuple: clean_test_db $(CONTEXTS) $(TEST_TUPLE_SQL) $(NAME_SE)
	@ ./$(NAME_SE) -quiet $(TEST_DB) $(TEST_TUPLE_SQL)

test_tuple_se: clean_test_db $(CONTEXTS) $(TEST_TUPLE_SE_SQL) $(NAME_SE)
	@ ./$(NAME_SE) -quiet $(TEST_DB) $(TEST_TUPLE_SE_SQL)

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

define generate_sql
	@ echo "[Generating $@]"
	tclsh $(TOP)/tool/$(1) > $@
endef

$(TEST_SQL):
	@ $(call generate_sql,mkspeedsql.tcl)

$(TEST_TUPLE_SQL):
	@ $(call generate_sql,mkspeedsql_tuple.tcl)

$(TEST_TUPLE_SE_SQL):
	@ $(call generate_sql,mkspeedsql_tuple_se.tcl)

clean_test_db:
	@- $(RM) $(TEST_DB)*

clean_temp: clean_test_db
	@- $(RM) $(NAME) $(NAME_SE) $(CONTEXTS)
	@- $(RM) $(OBJS) $(SE_SQLITE_OBJ) $(SQLITE_OBJ)

clean: clean_temp
	@- $(RM) -rf results

graph: $(BASE_OUT) $(SE_SQLITE_OUT) $(BASE_TUPLE_OUT) $(SE_SQLITE_TUPLE_OUT)
	@- python makegraph.py

results:
	@ mkdir -p results

define testout
	@ $(foreach i, $(shell seq 1 $(n)), echo "$(1) $(i) ..." ; make $(1) >> $@ ; )
endef

$(BASE_OUT): results $(TEST_SQL) $(NAME)
	@ $(call testout,test)

$(SE_SQLITE_OUT): results  $(CONTEXTS) $(TEST_SQL) $(NAME_SE)
	@ $(call testout,test_se)

$(BASE_TUPLE_OUT): results  $(CONTEXTS) $(TEST_TUPLE_SQL) $(NAME)
	@ $(call testout,test_tuple)

$(SE_SQLITE_TUPLE_OUT): results $(CONTEXTS) $(TEST_TUPLE_SE_SQL) $(NAME_SE)
	@ $(call testout,test_tuple_se)
