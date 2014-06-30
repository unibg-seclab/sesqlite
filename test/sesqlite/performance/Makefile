.PHONY: all clean test test_se clean_test_db

TOP             = ../../..
NAME           := speedtest8
NAME_SE        := $(NAME)_se
TEST_DB        := test.db
TEST_SQL       := test.sql
SQLITE_OBJ     := sqlite3.o
SE_SQLITE_OBJ  := sesqlite3.o

SRCS           := $(wildcard *.c)
OBJS           := $(SRCS:.c=.o)
INCLUDES       := -I$(TOP)/src
LDFLAGS        += -ldl


all: $(NAME) $(NAME_SE) $(TEST_SQL)

test: $(NAME) $(TEST_SQL) clean_test_db
	@./$(NAME) -quiet $(TEST_DB) $(TEST_SQL)

test_se: $(NAME_SE) $(TEST_SQL) clean_test_db
	@./$(NAME_SE) $(TEST_DB) $(TEST_SQL)

$(NAME): $(OBJS) $(SQLITE_OBJ)
	gcc $(OBJS) $(SQLITE_OBJ) -o $(NAME) $(INCLUDES) $(LDFLAGS)

$(NAME_SE): $(OBJS) $(SE_SQLITE_OBJ)
	gcc $(OBJS) $(SE_SQLITE_OBJ) -o $(NAME_SE) $(INCLUDES) $(LDFLAGS) -lselinux

$(SE_SQLITE_OBJ):
	@make performance-selinux -C $(TOP)
	cp $(TOP)/build/sqlite3.o ./$(SE_SQLITE_OBJ)

$(SQLITE_OBJ):
	@make performance -C $(TOP)
	cp $(TOP)/build/sqlite3.o ./$(SQLITE_OBJ)

$(TEST_SQL):
	@echo "[Generating $@]"
	tclsh $(TOP)/tool/mkspeedsql.tcl > $(TEST_SQL)

clean: clean_test_db
	@- $(RM) $(NAME) $(NAME_SE)
	@- $(RM) $(OBJS) $(SE_SQLITE_OBJ) $(SQLITE_OBJ)

clean_test_db:
	@- $(RM) $(TEST_DB)*
