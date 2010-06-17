WORKQUEUE_CFLAGS = -pthread -g3 -I../../lib
WORKQUEUE_LDLAGS = -pthread -lrt -lworkqueue -L../../lib

noinst_bin_PROGRAMS = ex_test_periodic
noinst_bindir = $(abs_top_builddir)/example_progs

ex_test_periodic_SOURCES = ex_test_periodic.c ex.c pthread_ex.c
ex_test_periodic_CFLAGS = $(AM_CFLAGS) $(WORKQUEUE_CFLAGS)
ex_test_periodic_LDFLAGS = $(WORKQUEUE_LDLAGS)
