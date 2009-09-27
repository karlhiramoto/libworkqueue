WORKQUEUE_CFLAGS = -pthread -g3 -I../../lib
WORKQUEUE_LDLAGS = -pthread -lrt -lworkqueue -L../../lib

noinst_bin_PROGRAMS = periodic_test
noinst_bindir = $(abs_top_builddir)/example_progs

periodic_test_SOURCES = periodic_test.c
periodic_test_CFLAGS = $(AM_CFLAGS) $(WORKQUEUE_CFLAGS)
periodic_test_LDFLAGS = $(WORKQUEUE_LDLAGS)
