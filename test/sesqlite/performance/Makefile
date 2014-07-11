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

$(TEST_SQL):
	@ echo "[Generating $@]"
	tclsh $(TOP)/tool/mkspeedsql.tcl > $@

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

$(BASE_OUT): results $(TEST_SQL) $(NAME)
	@ $(foreach i, $(shell seq 1 $(n)), echo "test $(i) ..." ; make test >> $@ ; )
