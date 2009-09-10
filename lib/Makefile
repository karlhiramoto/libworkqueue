PREFIX ?= /usr/local/libworkqueue
BINDIR ?= $(PREFIX)/bin
CFLAGS ?= -Wall -Os -g3

LIB_OBJECTS =  workqueue.o

.c.o:
	$(CC) -c $< $(CFLAGS) $(CC_DEFINES) $(CC_INCLUDES) -pthread

libworkqueue.a: $(LIB_OBJECTS)
	ar -r libworkqueue.a $(LIB_OBJECTS)

clean:
	rm -f *.a *.o