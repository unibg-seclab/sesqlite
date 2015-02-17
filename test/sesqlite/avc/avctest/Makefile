.PHONY: all test clean fresh fresh-test
.SUFFIXES: .c .o

n       := 10
NAME    := avctest
CFLAGS  += -O3
LDFLAGS += -lselinux

SRCS    := $(wildcard *.c)
OBJS    := $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	gcc $(CFLAGS) $(OBJS) -o $(NAME) $(LDFLAGS)

test: $(NAME)
	@./$(NAME) $(n)

clean:
	@- $(RM) $(NAME)
	@- $(RM) $(OBJS)

distclean: clean

fresh: | clean all

fresh-test: | clean test
