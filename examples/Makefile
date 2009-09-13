PREFIX ?= /usr/local/libworkqueue
BINDIR ?= $(PREFIX)/bin
CFLAGS ?= -Wall -Os -g3 -pedantic -std=c99

LIB = -lworkqueue -L../lib -lrt -pthread
CC_INCLUDES = -I../lib

#.c.o:
#	$(CC) -c $< $(CFLAGS) $(CC_DEFINES) $(CC_INCLUDES)

all: simple_test periodic_test
	touch examples.done

simple_test: simple_test.c
	$(CC) -o simple_test simple_test.c $(CFLAGS) $(CC_DEFINES) $(CC_INCLUDES) $(LIB) $(LDFLAGS)


periodic_test: periodic_test.c
	$(CC) -o periodic_test periodic_test.c $(CFLAGS) $(CC_DEFINES) $(CC_INCLUDES) $(LIB) $(LDFLAGS)

clean:
	rm -f *.a *.o examples.done
	rm -f simple_test periodic_test