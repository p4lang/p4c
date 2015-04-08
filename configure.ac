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

# Check for libjudy, libgmp, libnanomsg, libpcap
AC_CHECK_LIB([Judy], [Judy1Next], [], [AC_MSG_ERROR([Missing libJudy])])
AC_CHECK_LIB([gmp], [__gmpz_init], [], [AC_MSG_ERROR([Missing libgmp])])
AC_CHECK_LIB([nanomsg], [nn_errno], [], [AC_MSG_ERROR([Missing libnanomsg])])
AC_CHECK_LIB([pcap], [pcap_create], [], [AC_MSG_ERROR([Missing libpcap])])

# C++ libraries are harder (http://nerdland.net/2009/07/detecting-c-libraries-with-autotools/),
# so use headers to check
AC_CHECK_HEADER([boost/thread.hpp], [], [AC_MSG_ERROR([Boost threading headers not found])])
AC_CHECK_HEADER([boost/multiprecision/gmp.hpp], [], [AC_MSG_ERROR([Missing boost Multiprecision headers])])
AC_CHECK_HEADER([boost/program_options.hpp], [], [AC_MSG_ERROR([Missing boost program options header])])
AC_CHECK_HEADER([boost/functional/hash.hpp], [], [AC_MSG_ERROR([Missing boost functional hash header])])
AC_CHECK_HEADER([thrift/Thrift.h], [], [AC_MSG_ERROR([Thrift headers not found. Install Thrift from http://thrift.apache.org/docs/install/])])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T
AC_LANG_POP(C++)

# Generate makefiles
AC_CONFIG_FILES([Makefile
		thrift_src/Makefile
		third_party/Makefile
		third_party/gtest/Makefile
		third_party/jsoncpp/Makefile
		modules/Makefile
		modules/bf_lpm_trie/Makefile
		modules/bm_sim/Makefile
		modules/bm_runtime/Makefile
		modules/BMI/Makefile
		targets/Makefile
		targets/simple_router/Makefile
		tests/Makefile])
AC_OUTPUT
