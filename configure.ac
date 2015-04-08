#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([bm_sim], [0.1], [antonin@barefootnetworks.com])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_SRCDIR([modules/bm_sim/src/checksums.cpp])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

# Add picky flags
CXXFLAGS="-std=c++11"
PICKY_CXXFLAGS=""
PICKY_CFLAGS=""
AC_SUBST([PICKY_CXXFLAGS])
AC_SUBST([PICKY_CFLAGS])

# Checks for header files.
AC_LANG_PUSH(C)
AC_LANG_PUSH(C++)

AC_CHECK_HEADERS([algorithm array cassert cmath queue \
cstdio string sys/stat.h sys/types.h ctime tuple unistd.h unordered_map \
utility vector], [], [AC_MSG_ERROR([Missing header file])])

# Check for thrift support (Facebook's Thrift RPC library)
AC_PATH_PROG([THRIFT], [thrift], [])
AS_IF([test x"$THRIFT" = x],
[AC_MSG_ERROR([cannot find thrift])])

# Check for libnanomsg, libjsoncpp
PKG_CHECK_MODULES([libnanomsg], [libnanomsg])
PKG_CHECK_MODULES([jsoncpp], [jsoncpp])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T
AC_LANG_POP(C++)

# Generate makefiles
AC_CONFIG_FILES([Makefile
		modules/Makefile
		modules/bf_lpm_trie/Makefile
		modules/bm_sim/Makefile
		modules/bm_runtime/Makefile
		modules/BMI/Makefile
		thrift_src/Makefile
		tests/Makefile])
AC_OUTPUT
