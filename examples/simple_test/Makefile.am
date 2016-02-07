WORKQUEUE_CFLAGS = -pthread -g3 -I../../lib
WORKQUEUE_LDLAGS = -pthread -lworkqueue -lrt -L../../lib

noinst_bin_PROGRAMS = simple_test 
noinst_bindir = $(abs_top_builddir)/example_progs

simple_test_SOURCES = simple_test.c
simple_test_CFLAGS = $(AM_CFLAGS) $(WORKQUEUE_CFLAGS)
simple_test_LDFLAGS = $(WORKQUEUE_LDLAGS)
