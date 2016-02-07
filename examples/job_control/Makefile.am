WORKQUEUE_CFLAGS = -pthread -g3 -I../../lib
WORKQUEUE_LDLAGS = -pthread -lworkqueue -lrt -L../../lib

noinst_bin_PROGRAMS = job_control 
noinst_bindir = $(abs_top_builddir)/example_progs

job_control_SOURCES = job_control.c
job_control_CFLAGS = $(AM_CFLAGS) $(WORKQUEUE_CFLAGS)
job_control_LDFLAGS = $(WORKQUEUE_LDLAGS)
