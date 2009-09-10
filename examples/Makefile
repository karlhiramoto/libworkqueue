PREFIX ?= /usr/local/libworkqueue
BINDIR ?= $(PREFIX)/bin
CFLAGS ?= -Wall -Os -g3

LIB = -lworkqueue -L../lib -pthread
CC_INCLUDES = -I../lib

.c.o:
	$(CC) -c $< $(CFLAGS) $(CC_DEFINES) $(CC_INCLUDES)

all: simple_test
	touch examples.done

simple_test: simple_test.c
	$(CC) -o simple_test  simple_test.c $(CFLAGS) $(CC_DEFINES) $(CC_INCLUDES) $(LIB) $(LDFLAGS)


clean:
	rm -f *.a *.o examples.done
	rm -f simple_test 