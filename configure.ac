#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([bm_sim], [0.1], [antonin@barefootnetworks.com])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_SRCDIR([modules/bm_sim/src/checksums.cpp])
AC_CONFIG_HEADERS([config.h])

# Pyhton is optional to the package
AM_PATH_PYTHON([2.7],, [:])

coverage_enabled=no
AC_ARG_ENABLE([coverage],
    AS_HELP_STRING([--enable-coverage], [Enable code coverage tracking]))
AS_IF([test "x$enable_coverage" = "xyes"], [
    coverage_enabled=yes
    AC_DEFINE([COVERAGE], [], ["Link with gcov."])
    COVERAGE_FLAGS="-coverage"
])

AC_SUBST([COVERAGE_FLAGS])

AC_ARG_WITH([targets],
    AS_HELP_STRING([--without-targets], [Do not build targets]),
    [], [want_targets=yes])

AM_CONDITIONAL([COND_TARGETS], [test "$want_targets" = yes])

MY_CPPFLAGS=""

debugger_enabled=no
AC_ARG_ENABLE([debugger],
    AS_HELP_STRING([--enable-debugger], [Enable bmv2 remote debugger]))
AS_IF([test "x$enable_debugger" = "xyes"], [
    debugger_enabled=yes
    MY_CPPFLAGS="$MY_CPPFLAGS -DBMDEBUG_ON"
])

logging_macros_enabled=no
AC_ARG_ENABLE([logging_macros],
    AS_HELP_STRING([--disable-logging-macros],
                   [Disable compile time debug and trace logging macros]))
AS_IF([test "x$enable_logging_macros" != "xno"], [
    logging_macros_enabled=yes
    MY_CPPFLAGS="$MY_CPPFLAGS -DBMLOG_DEBUG_ON -DBMLOG_TRACE_ON"
])

# BMELOG_ON is defined by default, since it is required for some tests
elogger_enabled=no
AC_ARG_ENABLE([elogger],
    AS_HELP_STRING([--disable-elogger],
                   [Disable nanomsg event logger (some unit tests may fail)]))
AS_IF([test "x$enable_elogger" != "xno"], [
    elogger_enabled=yes
    MY_CPPFLAGS="$MY_CPPFLAGS -DBMELOG_ON"
])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
LT_INIT

AC_CONFIG_MACRO_DIR([m4])

# enforce -std=c++11
AX_CXX_COMPILE_STDCXX_11([noext],[mandatory])

# Add picky flags
PICKY_CXXFLAGS="-Wall -Werror -Wextra"
PICKY_CFLAGS=""
# I do not want work to use AC_DEFINE cause I do not want to include config.h
# It interfers with Thrift's config.h and it's too much work to do that cleanly
AC_SUBST([PICKY_CXXFLAGS])
AC_SUBST([PICKY_CFLAGS])

AC_SUBST([AM_CPPFLAGS], ["$MY_CPPFLAGS \
                          -I\$(top_srcdir)/modules/bf_lpm_trie/include \
                          -isystem\$(top_srcdir)/third_party/jsoncpp/include \
                          -isystem\$(top_srcdir)/third_party/spdlog/include \
                          -I\$(top_srcdir)/modules/BMI/include"])

# Checks for header files.
AC_LANG_PUSH(C)
AC_LANG_PUSH(C++)

# Thrift
AC_WITH([p4thrift],
    [
        AC_PATH_PROG([THRIFT], [p4thrift], [])
        AC_SUBST([THRIFT_LIB], ["-lp4thrift"])
        WITH_P4THRIFT="true"
        AC_CHECK_HEADER([p4thrift/P4Thrift.h], [], [AC_MSG_ERROR([P4Thrift headers not found. Install P4Thrift from http://github.com/p4lang/thrift/])])
    ],
    [
        AC_PATH_PROG([THRIFT], [thrift], [])
        AC_SUBST([THRIFT_LIB], ["-lthrift"])
        AC_CHECK_HEADER([thrift/Thrift.h], [], [AC_MSG_ERROR([Thrift headers not found. Install Thrift from http://thrift.apache.org/docs/install/])])
    ])
AM_CONDITIONAL([P4THRIFT], [test "x$WITH_P4THRIFT" != "x"])
AS_IF([test x"$THRIFT" = x],
[AC_MSG_ERROR([cannot find thrift])])


AC_CHECK_HEADERS([algorithm array cassert cmath queue \
cstdio string sys/stat.h sys/types.h ctime tuple unistd.h unordered_map \
utility vector], [], [AC_MSG_ERROR([Missing header file])])

# Check for libjudy, libgmp, libnanomsg, libpcap
AC_CHECK_LIB([Judy], [Judy1Next], [], [AC_MSG_ERROR([Missing libJudy])])
AC_CHECK_LIB([gmp], [__gmpz_init], [], [AC_MSG_ERROR([Missing libgmp])])
AC_CHECK_LIB([nanomsg], [nn_errno], [], [AC_MSG_ERROR([Missing libnanomsg])])
AC_CHECK_LIB([pcap], [pcap_create], [], [AC_MSG_ERROR([Missing libpcap])])
AC_CHECK_LIB([pcap], [pcap_set_immediate_mode], [pcap_fix=yes], [pcap_fix=no])
if test -n "$COVERAGE_FLAGS"; then
    AC_CHECK_LIB([gcov], [__gcov_init], [], [AC_MSG_ERROR([Missing gcov library])])
fi

AM_CONDITIONAL([WITH_PCAP_FIX], [test "$pcap_fix" = "yes"])

# C++ libraries are harder (http://nerdland.net/2009/07/detecting-c-libraries-with-autotools/),
# so use headers to check
AC_CHECK_HEADER([boost/thread.hpp], [], [AC_MSG_ERROR([Boost threading headers not found])])
AC_CHECK_HEADER([boost/multiprecision/gmp.hpp], [], [AC_MSG_ERROR([Missing boost Multiprecision headers])])
AC_CHECK_HEADER([boost/program_options.hpp], [], [AC_MSG_ERROR([Missing boost program options header])])
AC_CHECK_HEADER([boost/functional/hash.hpp], [], [AC_MSG_ERROR([Missing boost functional hash header])])
AC_CHECK_HEADER([boost/filesystem.hpp], [], [AC_MSG_ERROR([Missing boost filesystem header])])

# Checks for typedefs, structures, and compiler characteristics.
# not supported by autoconf 2.68, add to m4/ ?
# AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T
AC_LANG_POP(C++)

# Generate makefiles
AC_CONFIG_FILES([Makefile
		thrift_src/Makefile
		third_party/Makefile
		third_party/gtest/Makefile
		third_party/jsoncpp/Makefile
		third_party/spdlog/Makefile
		modules/Makefile
		modules/bf_lpm_trie/Makefile
		modules/bm_sim/Makefile
		modules/bm_runtime/Makefile
		modules/BMI/Makefile
		modules/bm_apps/Makefile
		modules/bm_apps/include/Makefile
		modules/bm_apps/examples/Makefile
		targets/Makefile
		targets/simple_router/Makefile
		targets/l2_switch/Makefile
		targets/l2_switch/learn_client/Makefile
		targets/simple_switch/Makefile
		targets/simple_switch/tests/Makefile
		tests/Makefile])

AC_OUTPUT

AS_ECHO("")
AS_ECHO("Features recap ......................")
AS_ECHO("Coverage enabled .............. : $coverage_enabled")
AS_ECHO("Debugger enabled .............. : $debugger_enabled")
AS_ECHO("Logging macros enabled ........ : $logging_macros_enabled")
AS_ECHO("Event logger enabled .......... : $elogger_enabled")
