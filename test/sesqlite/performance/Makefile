.PHONY: all clean clean_test_db \
	graph results_base results_se \
	graph1 test1 test1_se \
	graph8 test8 test8_se test8_tuple test8_tuple_static \
	graphrl testrl testrl_se \
	graphnum testnum testnum_se

n			:= 2
TOP			:= ../../..
CONTEXTS		:= sesqlite_contexts
SQLITE_OBJ		:= sqlite3.o
SQLITE_LIB		:= libsqlite3.so
SE_SQLITE_OBJ	:= sesqlite3.o
SE_SQLITE_LIB	:= libsesqlite3.so
INCLUDES		:= -I$(TOP)/src
LDFLAGS			+= -ldl

### speedtest1

SRCS1			:= speedtest1.c
OBJS1			:= $(SRCS1:.c=.o)
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

### speedtest_rl

BINRL			:= speedtestrl.py
TESTRL_DB		:= test_rl.db
OUTDIRRL		:= results/rl
OUTRL_BASE		:= $(OUTDIRRL)/1.base.out
OUTRL_SE		:= $(OUTDIRRL)/2.se.out

### speedtest_num

BINNUM			:= speedtestnum.py
OUTDIRNUM		:= results/num
OUTNUM_BASE		:= $(OUTDIRNUM)/1.base.out
OUTNUM_SE		:= $(OUTDIRNUM)/2.se.out
NUM_FROM		:= 0
NUM_TO			:= 100000
NUM_STEP		:= 10000
NUM_REP			:= 10
NUM_DROP		:= 2
NUM_INLIMIT		:= 1000


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

$(SQLITE_OBJ) $(SQLITE_LIB):
	@ make performance -C $(TOP)
	cp $(TOP)/build/sqlite3.o ./$(SQLITE_OBJ)
	cp $(TOP)/build/.libs/libsqlite3.so ./$(SQLITE_LIB)

$(SE_SQLITE_OBJ) $(SE_SQLITE_LIB):
	@ make performance-selinux -C $(TOP)
	cp $(TOP)/build/sqlite3.o ./$(SE_SQLITE_OBJ)
	cp $(TOP)/build/.libs/libsqlite3.so ./$(SE_SQLITE_LIB)

$(CONTEXTS):
	cp $(TOP)/test/sesqlite/policy/$@ .

graph: graph1 graph8 graphrl

results_base: $(OUT1_BASE) $(OUT8_BASE) $(OUTRL_BASE)

results_se: $(OUT1_SE) $(OUT8_SE) $(OUT8_TUPLE) $(OUT8_TUPLE_STATIC) $(OUTRL_SE)

clean_test_db:
	@- $(RM) $(TEST8_DB)* $(TEST1_DB)* $(TESTRL_DB)*

clean: clean_test_db
	@- $(RM) $(SQLITE_OBJ) $(SE_SQLITE_OBJ)
	@- $(RM) $(SQLITE_LIB) $(SE_SQLITE_LIB)
	@- $(RM) $(BIN1) $(BIN1_SE) $(BIN8) $(BIN8_SE)
	@- $(RM) $(OBJS1) $(OBJS8)
	@- $(RM) $(CONTEXTS)
	@- $(RM) -rf results


#########################
#  SPEEDTEST1 SPECIFIC  #
#########################

$(BIN1): $(SQLITE_OBJ) $(OBJS1)
	gcc -g $(SQLITE_OBJ) $(OBJS1) -o $@ $(INCLUDES) $(LDFLAGS)

$(BIN1_SE): $(SE_SQLITE_OBJ) $(OBJS1)
	gcc -g $(SE_SQLITE_OBJ) $(OBJS1) -o $@ $(INCLUDES) $(LDFLAGS) -lselinux

### tests

test1: clean_test_db $(BIN1)
	@ ./$(BIN1) $(TEST1_DB)

test1_se: clean_test_db $(BIN1_SE) $(CONTEXTS)
	@ ./$(BIN1_SE) $(TEST1_DB)

### result files

$(OUTDIR1):
	@ mkdir -p $@

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
	gcc -g $(OBJS8) $(SQLITE_OBJ) -o $@ $(INCLUDES) $(LDFLAGS)

$(BIN8_SE): $(OBJS8) $(SE_SQLITE_OBJ)
	gcc -g $(OBJS8) $(SE_SQLITE_OBJ) -o $@ $(INCLUDES) $(LDFLAGS) -lselinux

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

##########################
#  SPEEDTESTRL SPECIFIC  #
##########################

### tests

testrl: clean_test_db $(SQLITE_LIB) $(BINRL)
	@ LD_PRELOAD=./$(SQLITE_LIB) python $(BINRL) $(TESTRL_DB)

testrl_se: clean_test_db $(CONTEXTS) $(SE_SQLITE_LIB) $(BINRL)
	@ LD_PRELOAD=./$(SE_SQLITE_LIB) python $(BINRL) $(TESTRL_DB)

### result files

$(OUTDIRRL):
	@ mkdir -p $@

$(OUTRL_BASE): $(OUTDIRRL) $(SQLITE_LIB)
	@ $(call testout,testrl)

$(OUTRL_SE): $(OUTDIRRL) $(CONTEXTS) $(SE_SQLITE_LIB)
	@ $(call testout,testrl_se)

# graph

graphrl: $(OUTRL_BASE) $(OUTRL_SE)
	@ python makegraphrl.py

###########################
#  SPEEDTESTNUM SPECIFIC  #
###########################

### tests

testnum: $(SQLITE_LIB) $(BINNUM)
	@ LD_PRELOAD=./$(SQLITE_LIB) python $(BINNUM) -f$(NUM_FROM) -t$(NUM_TO) -s$(NUM_STEP) -r$(NUM_REP) -d$(NUM_DROP) -i$(NUM_INLIMIT)

testnum_se: $(CONTEXTS) $(SE_SQLITE_LIB) $(BINNUM)
	@ LD_PRELOAD=./$(SE_SQLITE_LIB) python $(BINNUM) -f$(NUM_FROM) -t$(NUM_TO) -s$(NUM_STEP) -r$(NUM_REP) -d$(NUM_DROP) -i$(NUM_INLIMIT)

### result files

$(OUTDIRNUM):
	@ mkdir -p $@

$(OUTNUM_BASE): $(OUTDIRNUM) $(SQLITE_LIB) $(BINNUM)
	make -s testnum > $(OUTNUM_BASE)

$(OUTNUM_SE): $(OUTDIRNUM) $(CONTEXTS) $(SE_SQLITE_LIB) $(BINNUM)
	make -s testnum_se > $(OUTNUM_SE)

# graph

graphnum: $(OUTNUM_BASE) $(OUTNUM_SE)
	@ python makegraphnum.py $(OUTNUM_BASE) $(OUTNUM_SE) --outdir $(OUTDIRNUM)

