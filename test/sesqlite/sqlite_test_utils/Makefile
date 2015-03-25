.PHONY: all clean test

SRCS    = $(wildcard *.c)
OBJS    = $(SRCS:.c=.o)
BIN     = example

all: $(BIN)

$(BIN): $(OBJS)
	gcc -g $(OBJS) -o $@ -lsqlite3

run: $(BIN)
	@ ./$(BIN) ||:

clean:
	@ rm -rf $(OBJS) $(BIN)

