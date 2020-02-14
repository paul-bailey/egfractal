bin_PROGRAMS = \
  mbrot2
LDADD = $(top_srcdir)/lib/libfractal.a -lpthread
mbrot2_SOURCES = \
   palette.c \
   parse_args.c \
   mandelbrot_common.h \
   mbrot_thread.c \
   main.c
mbrot2_CPPFLAGS = -I$(top_srcdir)/include -Wall -std=gnu11 -O3

