.PHONY: all clean clean_test_db \
	graph results_base results_se \
	test1 test1_se \
	test8 test8_se test8_tuple test8_tuple_static

n			:= 2
TOP			:= ../../..
CONTEXTS		:= sesqlite_contexts
SQLITE_OBJ		:= sqlite3.o
SE_SQLITE_OBJ		:= sesqlite3.o
INCLUDES		:= -I$(TOP)/src
LDFLAGS			+= -ldl

### speedtest1

SRCS1			:= speedtest1.c
SRCS1_SE		:= speedtest1_se.c
OBJS1			:= $(SRCS1:.c=.o)
OBJS1_SE		:= $(SRCS1_SE:.c=.o)
BIN1			:= speedtest1
BIN1_SE			:= $(BIN1)_se
TEST1_DB		:= test1.db
OUTDIR1			:= results/1
OUT1_BASE		:= $(OUTDIR1)/1.base.out
OUT1_SE			:= $(OUTDIR1)/2.se.out

### speedtest8

SRCS8			:= speedtest8.c ts_util.c
OBJS8			:= $(SRCS8:.c=.o)
BIN8			:= speedtest8
BIN8_SE			:= $(BIN8)_se
TEST8_DB		:= test8.db
TEST8_SQL		:= test.sql
TEST8_TUPLE_SQL		:= test_tuple.sql
TEST8_TUPLE_STATIC_SQL	:= test_tuple_static.sql
OUTDIR8			:= results/8
OUT8_BASE		:= $(OUTDIR8)/1.base.out
OUT8_SE			:= $(OUTDIR8)/2.se.out
OUT8_TUPLE		:= $(OUTDIR8)/4.se_tuple.out
OUT8_TUPLE_STATIC	:= $(OUTDIR8)/3.se_tuple_static.out


#########################
#      COMMON PART      #
#########################

define testout
	@ make clean_test_db
	@ $(foreach i, $(shell seq 1 $(n)), echo "$(1) $(i) ..." ; make $(1) >> $@ ; )
endef

define generate_sql
	@ echo "[Generating $@]"
	tclsh $(TOP)/tool/$(1) > $@
endef

all: $(BIN1) $(BIN1_SE) \
     $(BIN8) $(BIN8_SE) \
     $(CONTEXTS) \
     $(TEST8_SQL) $(TEST8_TUPLE_SQL) $(TEST8_TUPLE_STATIC_SQL)

$(SQLITE_OBJ):
	@ make performance -C $(TOP)
	cp $(TOP)/build/sqlite3.o ./$(SQLITE_OBJ)

$(SE_SQLITE_OBJ):
	@ make performance-selinux -C $(TOP)
	cp $(TOP)/build/sqlite3.o ./$(SE_SQLITE_OBJ)

$(CONTEXTS):
	cp $(TOP)/test/sesqlite/policy/$@ .

graph: graph1 graph8

results_base: $(OUT1_BASE) $(OUT8_BASE)

results_se: $(OUT1_SE) $(OUT8_SE) $(OUT8_TUPLE) $(OUT8_TUPLE_STATIC)

clean_test_db:
	@- $(RM) $(TEST8_DB)* $(TEST1_DB)*

clean: clean_test_db
	@- $(RM) $(BIN1) $(BIN1_SE) $(BIN8) $(BIN8_SE)
	@- $(RM) $(OBJS1) $(OBJS8) $(SQLITE_OBJ) $(SE_SQLITE_OBJ)
	@- $(RM) $(CONTEXTS)
	@- $(RM) -rf results


#########################
#  SPEEDTEST1 SPECIFIC  #
#########################

$(BIN1): $(SQLITE_OBJ) $(OBJS1)
	gcc $(SQLITE_OBJ) $(OBJS1) -o $@ $(INCLUDES) $(LDFLAGS)

$(BIN1_SE): $(SE_SQLITE_OBJ) $(OBJS1_SE)
	gcc $(SE_SQLITE_OBJ) $(OBJS1_SE) -o $@ $(INCLUDES) $(LDFLAGS) -lselinux

### tests

test1: clean_test_db $(BIN1)
	@ ./$(BIN1) $(TEST1_DB)

test1_se: clean_test_db $(BIN1_SE) $(CONTEXTS)
	@ ./$(BIN1_SE) $(TEST1_DB)

### result files

$(OUTDIR1):
	mkdir -p $@

$(OUT1_BASE): $(OUTDIR1) $(BIN1)
	@ $(call testout,test1)

$(OUT1_SE): $(OUTDIR1) $(BIN1_SE)
	@ $(call testout,test1_se)

# graph

graph1: $(OUT1_BASE) $(OUT1_SE)
	@ python makegraph1.py


#########################
#  SPEEDTEST8 SPECIFIC  #
#########################

$(BIN8): $(OBJS8) $(SQLITE_OBJ)
	gcc $(OBJS8) $(SQLITE_OBJ) -o $@ $(INCLUDES) $(LDFLAGS)

$(BIN8_SE): $(OBJS8) $(SE_SQLITE_OBJ)
	gcc $(OBJS8) $(SE_SQLITE_OBJ) -o $@ $(INCLUDES) $(LDFLAGS) -lselinux

### sql statements

$(TEST8_SQL):
	@ $(call generate_sql,mkspeedsql.tcl)

$(TEST8_TUPLE_SQL):
	@ $(call generate_sql,mkspeedsql_tuple.tcl)

$(TEST8_TUPLE_STATIC_SQL):
	@ $(call generate_sql,mkspeedsql_tuple_static.tcl)

### tests

test8: clean_test_db $(TEST8_SQL) $(BIN8)
	@ ./$(BIN8) -quiet $(TEST8_DB) $(TEST8_SQL)

test8_se: clean_test_db $(CONTEXTS) $(TEST8_SQL) $(BIN8_SE)
	@ ./$(BIN8_SE) -quiet $(TEST8_DB) $(TEST8_SQL)

test8_tuple: clean_test_db $(CONTEXTS) $(TEST8_TUPLE_SQL) $(BIN8_SE)
	@ ./$(BIN8_SE) -quiet $(TEST8_DB) $(TEST8_TUPLE_SQL)

test8_tuple_static: clean_test_db $(CONTEXTS) $(TEST8_TUPLE_STATIC_SQL) $(BIN8_SE)
	@ ./$(BIN8_SE) -quiet $(TEST8_DB) $(TEST8_TUPLE_STATIC_SQL)

### result files

$(OUTDIR8):
	@ mkdir -p $@

$(OUT8_BASE): $(OUTDIR8) $(TEST8_SQL) $(BIN8)
	@ $(call testout,test8)

$(OUT8_SE): $(OUTDIR8) $(CONTEXTS) $(TEST8_SQL) $(BIN8_SE)
	@ $(call testout,test8_se)

$(OUT8_TUPLE): $(OUTDIR8) $(CONTEXTS) $(TEST8_TUPLE_SQL) $(BIN8_SE)
	@ $(call testout,test8_tuple)

$(OUT8_TUPLE_STATIC): $(OUTDIR8) $(CONTEXTS) $(TEST8_TUPLE_STATIC_SQL) $(BIN8_SE)
	@ $(call testout,test8_tuple_static)

### graph

graph8: $(OUT8_BASE) $(OUT8_SE) $(OUT8_TUPLE) $(OUT8_TUPLE_STATIC)
	@ python makegraph8.py

