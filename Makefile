PREFIX ?= /usr/local/libworkqueue
BINDIR ?= $(PREFIX)/bin

FLAGS= -C

lib: lib/libworkqueue.a
	$(MAKE) $(FLAGS) lib/ $(MAKEFLAGS)

examples/periodic_test examples/simple_test: lib/libworkqueue.a
	$(MAKE) $(FLAGS) examples/ $(MAKEFLAGS)

examples: examples/simple_test examples/periodic_test
	


clean:
	$(MAKE) $(FLAGS) lib/ clean $(MAKEFLAGS)
	$(MAKE) $(FLAGS) examples/ clean $(MAKEFLAGS)

rebuild: clean examples


lib/libworkqueue.a: lib/workqueue.c lib/workqueue.h
	$(MAKE) $(FLAGS) lib/ $(MAKEFLAGS)
