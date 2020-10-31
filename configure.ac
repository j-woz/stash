
# STASH: CONFIGURE AC

AC_PREREQ([2.69])
AC_INIT([stash], [1.0], [woz@anl.gov])
AM_INIT_AUTOMAKE([subdir-objects])
# AM_INIT_AUTOMAKE() # may need this to bootstrap
AC_CONFIG_SRCDIR([src/stash.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stddef.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_MODE_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([ftruncate strchr strdup strstr strtol])

VALGRIND=0
AC_ARG_ENABLE(valgrind,[Enable valgrind special features],
  [if test "$enableval" = "yes" ; then VALGRIND=1 ; fi])
AC_DEFINE_UNQUOTED([VALGRIND],$VALGRIND,
  [If 1, use valgrind special features.])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
