#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([tm_sim], [0.1], [anirudh@barefootnetworks.com])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src/buffer_manager.cc])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_RANLIB

# Add picky CXXFLAGS
CPPFLAGS="-std=c++11 -pthread"
PICKY_CXXFLAGS="-pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline"
AC_SUBST([PICKY_CXXFLAGS])

# Google Unit Test framework
AC_ARG_VAR(GTEST_DIR, "Root directory of Google Unit Test")
AC_SUBST([GTEST_DIR])

# OMNeT++ simulation framework
AC_ARG_VAR(OMNETPP_DIR, "Root directory of OMNeT++ simulation framework")
AC_SUBST([OMNETPP_DIR])

# Checks for header files.
AC_LANG_PUSH(C++)

AC_CHECK_HEADERS([algorithm array cassert cmath queue \
cstdio string sys/stat.h sys/types.h ctime tuple unistd.h unordered_map \
utility vector], [], [AC_MSG_ERROR([Missing header file])])

# Check for protobuf support
AC_PATH_PROG([PROTOC], [protoc], [])
AS_IF([test x"$PROTOC" = x],
[AC_MSG_ERROR([cannot find protoc, the Protocol Buffers compiler])])
PKG_CHECK_MODULES([protobuf], [protobuf])

# Check for opp_msgc support (OMNeT's header library)
AC_PATH_PROG([MSGC], [opp_msgc], [])
AS_IF([test x"$MSGC" = x],
[AC_MSG_ERROR([cannot find opp_msgc, the OMNeT message compiler])])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T
AC_LANG_POP(C++)

# Checks for library functions.

AC_CONFIG_FILES([Makefile
		src/Makefile
		tests/Makefile
		protobufs/Makefile
		tools/Makefile
		omnet/Makefile
		omnet/msgs/Makefile
		omnet/src/Makefile])
AC_OUTPUT
