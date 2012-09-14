
AC_PREREQ(2.63)

AC_INIT([socfw], [0.1])
AC_CONFIG_SRCDIR([src/socfw.c])
AC_CONFIG_MACRO_DIR([m4])
#AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign 1.11 -Wall -Wno-portability])

AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_GCC_TRADITIONAL
AC_USE_SYSTEM_EXTENSIONS
AM_PROG_CC_C_O

AC_CHECK_PROGS([M4], gm4 m4, no)
if test "x$M4" = xno ; then
   AC_MSG_ERROR([m4 missing])
fi

CC_NOUNDEFINED

LT_PREREQ(2.2)
LT_INIT([dlopen])

AC_HEADER_STDC

AC_C_CONST
AC_C_BIGENDIAN
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_OFF_T
AC_TYPE_UID_T

# check for dlopen
AC_SEARCH_LIBS([dlopen], [dl])

AC_ARG_WITH(
        [kernel-dir],
        AS_HELP_STRING([--with-linux-dir],[linux source code directory]),
        [LINUX_DIR=$withval], [LINUX_DIR="~/source/linux.git"])
AC_SUBST(LINUX_DIR)

AC_ARG_ENABLE([omap4],
    AS_HELP_STRING([--enable-omap4],[Enable optional OMAP4+ output support]))

AS_IF([test "x$enable_omap4" = "xyes"],
    HAVE_OMAP=1,
    HAVE_OMAP=0)

AC_SUBST(HAVE_OMAP)
AM_CONDITIONAL([HAVE_OMAP], [test "x$enable_omap4" = "xyes"])

AC_CONFIG_FILES([
Makefile
src/Makefile
plugins/Makefile
plugins/omap4plus/Makefile
plugins/omap4plus/coeffs/Makefile
plugins/omap4plus/dapm/Makefile
])

AC_OUTPUT

