#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([simple-switch-grpc], [0.1], [antonin@barefootnetworks.com])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_HEADERS([config.h])

MY_CPPFLAGS=""

AC_PROG_CC
AC_PROG_CXX

AM_PATH_PYTHON([2.7],, [:])

LT_INIT

AC_CONFIG_MACRO_DIR([m4])

AC_LANG_PUSH(C++)

AX_CXX_COMPILE_STDCXX_11([noext],[mandatory])

# check for pthreads
AX_PTHREAD([], [AC_MSG_ERROR([Missing pthread library])])

AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_SIZE_T

AC_SUBST([AM_CPPFLAGS], ["$MY_CPPFLAGS"])
AC_SUBST([AM_CFLAGS], ["$PTHREAD_CFLAGS -Wall -Werror -Wextra"])

PKG_CHECK_MODULES([PROTOBUF], [protobuf >= 3.0.0])
AC_SUBST([PROTOBUF_CFLAGS])
AC_SUBST([PROTOBUF_LIBS])

PKG_CHECK_MODULES([GRPC], [grpc >= 1.0.0 grpc++ >= 1.0.0])
AC_SUBST([GRPC_CFLAGS])
AC_SUBST([GRPC_LIBS])

# Generate makefiles
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
