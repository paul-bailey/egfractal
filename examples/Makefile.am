examples_cppflags = -I$(top_srcdir)/include -Wall
bin_PROGRAMS = \
    dragon \
    sierpinski
LDADD = $(top_srcdir)/lib/libfractal.a
sierpinski_SOURCES = sierpinski.c
sierpinski_CPPFLAGS = $(examples_cppflags)
dragon_SOURCES = dragon.c
dragon_CPPFLAGS = $(examples_cppflags)

