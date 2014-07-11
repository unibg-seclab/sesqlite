.PHONY: all clean clean_test_db graph test

n					 = 2
TOP					 = ../../..
NAME				:= speedtest8
TEST_DB				:= test.db
TEST_SQL			:= test.sql

SQLITE_OBJ			:= sqlite3.o

SRCS				:= $(wildcard *.c)
OBJS				:= $(SRCS:.c=.o)
INCLUDES			:= -I$(TOP)/src
LDFLAGS				+= -ldl

BASE_OUT			 = results/base.out


all: $(NAME) $(TEST_SQL)

test: clean_test_db $(TEST_SQL) $(NAME)
	@ ./$(NAME) -quiet $(TEST_DB) $(TEST_SQL)

$(NAME): $(OBJS) $(SQLITE_OBJ)
	gcc $(OBJS) $(SQLITE_OBJ) -o $(NAME) $(INCLUDES) $(LDFLAGS)

$(SQLITE_OBJ):
	@ make -C $(TOP)
	cp $(TOP)/build/sqlite3.o ./$(SQLITE_OBJ)

define generate_sql
	@ echo "[Generating $@]"
	tclsh $(TOP)/tool/$(1) > $@
endef

$(TEST_SQL):
	@ $(call generate_sql, mkspeedsql.tcl)

clean_test_db:
	@- $(RM) $(TEST_DB)*

clean: clean_test_db
	@- $(RM) $(NAME)
	@- $(RM) $(OBJS) $(SQLITE_OBJ)
	@- $(RM) -rf results

graph: $(BASE_OUT)
	@- python makegraph.py

results:
	@ mkdir results

define testout
	@ $(foreach i, $(shell seq 1 $(n)), echo "$(1) $(i) ..." ; make $(1) >> $@ ; )
endef

$(BASE_OUT): results $(TEST_SQL) $(NAME)
	@ $(call testout, test)
