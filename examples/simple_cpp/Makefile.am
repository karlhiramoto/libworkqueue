WORKQUEUE_CXXFLAGS = -pthread -g3 -I../../lib
WORKQUEUE_LDLAGS =  -lworkqueue -lpthread -lrt -pthread -L../../lib

noinst_bin_PROGRAMS = simple
noinst_bindir = $(abs_top_builddir)/example_progs

simple_SOURCES = simple.cpp
simple_CXXFLAGS = $(AM_CFLAGS) $(WORKQUEUE_CXXFLAGS)
simple_LDFLAGS = $(WORKQUEUE_LDLAGS)
