# TESTS = simple_test periodic_test serial_port_test

WORKQUEUE_CFLAGS = -pthread -g3 -I../lib
WORKQUEUE_LDLAGS = -pthread -lrt -lworkqueue -L../lib

noinst_bin_PROGRAMS = simple_test periodic_test serial_port_test high_load_test
noinst_bindir = $(abs_top_builddir)/example_progs

high_load_test_SOURCES = high_load_test.c
high_load_test_CFLAGS = $(AM_CFLAGS) $(WORKQUEUE_CFLAGS)
high_load_test_LDFLAGS = $(WORKQUEUE_LDLAGS)

simple_test_SOURCES = simple_test.c
simple_test_CFLAGS = $(AM_CFLAGS) $(WORKQUEUE_CFLAGS)
simple_test_LDFLAGS = $(WORKQUEUE_LDLAGS)


periodic_test_SOURCES = periodic_test.c
periodic_test_CFLAGS = $(AM_CFLAGS) $(WORKQUEUE_CFLAGS)
periodic_test_LDFLAGS = $(WORKQUEUE_LDLAGS)


serial_port_test_SOURCES = serial_port_test.c
serial_port_test_CFLAGS = $(AM_CFLAGS) $(WORKQUEUE_CFLAGS)
serial_port_test_LDFLAGS = $(WORKQUEUE_LDLAGS)