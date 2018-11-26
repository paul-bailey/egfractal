examples_cppflags = -I$(top_srcdir)/include -Wall
bin_PROGRAMS = \
    dots \
    random \
    playpen \
    mbrot2 \
    koch
LDADD = $(top_srcdir)/lib/libfractal.a
dots_SOURCES = dots.c
random_SOURCES = random.c
playpen_SOURCES = playpen.c
mbrot2_SOURCES = mbrot2.c
koch_SOURCES = koch.c
dots_CPPFLAGS = $(examples_cppflags)
random_CPPFLAGS = $(examples_cppflags)
playpen_CPPFLAGS = $(examples_cppflags)
mbrot2_CPPFLAGS = $(examples_cppflags)
koch_CPPFLAGS = $(examples_cppflags)

