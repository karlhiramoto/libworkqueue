WORKQUEUE_CFLAGS = -pthread -g3 -I../../lib
WORKQUEUE_LDLAGS = -pthread -lrt -lworkqueue -L../../lib

noinst_bin_PROGRAMS = time_sort
noinst_bindir = $(abs_top_builddir)/example_progs

time_sort_SOURCES = time_sort.c
time_sort_CFLAGS = $(AM_CFLAGS) $(WORKQUEUE_CFLAGS)
time_sort_LDFLAGS = $(WORKQUEUE_LDLAGS)
