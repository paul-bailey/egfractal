AC_PREREQ([2.68])
AC_INIT([roscid-fractal], [1.0], [roscidcup@falstaffandme.com])

AC_CONFIG_AUX_DIR([build])
AC_CONFIG_MACRO_DIR([build/m4])
AC_CONFIG_SRCDIR([mandelbrot/mandelbrot.c])
AC_CONFIG_HEADERS([config.h])

LT_PREREQ([2.2])
LT_INIT([dlopen])

AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_CC_STDC
# I don't really meand to install this, but...
AC_PROG_INSTALL
AM_PROG_CC_C_O
AC_SEARCH_LIBS([sqrt], [m])

AC_CHECK_HEADERS([unistd.h stdio.h stdlib.h string.h stdint.h \
                  ctype.h errno.h assert.h math.h], , \
                  [AC_MSG_ERROR([Could not find a necessary header])])

AC_HEADER_STDBOOL
AC_C_INLINE

# TODO:
# AC_CHECK_FUNCS([...whatever lib functions I need], , [AC_MSG_ERR...

AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 random/Makefile
                 examples/Makefile
                 burnship1/Makefile
                 mandelbrot/Makefile])

AC_OUTPUT

echo \
"-------------------------------------------------

${PACKAGE_NAME} Version ${PACKAGE_VERSION}

Prefix: '${prefix}'.
Compiler: '${CC} ${CFLAGS} ${CPPFLAGS}'

Now type 'make @<:@<target>@:>@'
    where the optional <target> is:
        all         - build all binaries
        install     - install everything

--------------------------------------------------"


