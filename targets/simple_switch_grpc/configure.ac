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

PKG_CHECK_MODULES([GRPC], [grpc++ >= 1.3.0 grpc >= 3.0.0])
AC_SUBST([GRPC_CFLAGS])
AC_SUBST([GRPC_LIBS])

AC_PATH_PROG([PROTOC], [protoc], [])
AS_IF([test "x$PROTOC" = x], [AC_MSG_ERROR([protoc not found])])

AC_PATH_PROG([GRPC_CPP_PLUGIN], [grpc_cpp_plugin])
AS_IF([test "x$GRPC_CPP_PLUGIN" = x], [
    AC_MSG_ERROR([grpc_cpp_plugin not found])
])
AS_IF([test "$PYTHON" != :], [
    AC_PATH_PROG([GRPC_PY_PLUGIN], [grpc_python_plugin])
    AS_IF([test "x$GRPC_PY_PLUGIN" = x], [
        AC_MSG_WARN([grpc_python_plugin not found, Python code won't be generated])
    ])
])
AM_CONDITIONAL([HAVE_GRPC_PY_PLUGIN], [test "x$GRPC_PY_PLUGIN" != x])

AC_ARG_WITH([sysrepo],
    AS_HELP_STRING([--with-sysrepo],
                   [Use sysrepo gNMI service implementation @<:@default=no@:>@]),
    [with_sysrepo="$withval"], [with_sysrepo=no])
AM_CONDITIONAL([WITH_SYSREPO], [test "$with_sysrepo" = yes])
AM_COND_IF([WITH_SYSREPO], [
    AC_CHECK_LIB([sysrepo], [sr_connect], [],
                 [AC_MSG_ERROR([Missing libsysrepo])])
])

# Generate makefiles
AC_CONFIG_FILES([Makefile
                 tests/Makefile])

AC_CONFIG_FILES([tests/example.run], [chmod +x tests/example.run])

AC_OUTPUT
