WORKQUEUE_CFLAGS = -pthread -g3 -I../../lib
WORKQUEUE_LDLAGS = -pthread -lrt -lworkqueue -L../../lib

noinst_bin_PROGRAMS = serial_port_test
noinst_bindir = $(abs_top_builddir)/example_progs

serial_port_test_SOURCES = serial_port_test.c
serial_port_test_CFLAGS = $(AM_CFLAGS) $(WORKQUEUE_CFLAGS)
serial_port_test_LDFLAGS = $(WORKQUEUE_LDLAGS)
