#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([bm], [m4_esyscmd(tools/get_version.sh)],
        [antonin@barefootnetworks.com])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_SRCDIR([src/bm_sim/checksums.cpp])
AC_CONFIG_HEADERS([config.h])

AC_SUBST([BM_VERSION], [AC_PACKAGE_VERSION])

# Python is optional to the package
m4_define_default([_AM_PYTHON_INTERPRETER_LIST], [python3 python2 python2.7])
AM_PATH_PYTHON([3.5],, [:])

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

want_stress_tests=no
AC_ARG_WITH([stress_tests],
    AS_HELP_STRING([--with-stress-tests], [Include stress tests]),
    [want_stress_tests=yes], [])

AM_CONDITIONAL([COND_STRESS_TESTS], [test "$want_stress_tests" = yes])

want_pdfixed=no
AC_ARG_WITH([pdfixed],
    AS_HELP_STRING([--with-pdfixed], [Build pdfixed for bmv2]),
    [want_pdfixed=yes], [])

AM_CONDITIONAL([COND_PDFIXED], [test "$want_pdfixed" = yes])

AC_ARG_WITH([nanomsg],
    AS_HELP_STRING([--with-nanomsg], [Build Nanomsg RPC service, if disabled then you must have some other way of controlling the switch]),
    [want_nanomsg="$withval"], [want_nanomsg=yes])

AM_CONDITIONAL([COND_NANOMSG], [test "$want_nanomsg" = yes])

debugger_enabled=no
AC_ARG_ENABLE([debugger],
    AS_HELP_STRING([--enable-debugger], [Enable bmv2 remote debugger]))
AS_IF([test "x$enable_debugger" = "xyes"], [
    AS_IF([test "$want_nanomsg" = "yes"], [
        debugger_enabled=yes
        AC_DEFINE([DEBUG_ON], [], [Enable debugger])
    ], [
        AC_MSG_ERROR([Cannot use debugger without nanomsg])
    ])
])

logging_macros_enabled=no
AC_ARG_ENABLE([logging_macros],
    AS_HELP_STRING([--disable-logging-macros],
                   [Disable compile time debug and trace logging macros]))
AS_IF([test "x$enable_logging_macros" != "xno"], [
    logging_macros_enabled=yes
    AC_DEFINE([LOG_DEBUG_ON], [], [Enable compile-time macro for debug logging])
    AC_DEFINE([LOG_TRACE_ON], [], [Enable compile-time macro for trace logging])
])

# BM_ELOG_ON is defined by default, since it is required for some tests
elogger_enabled=no
AC_ARG_ENABLE([elogger],
    AS_HELP_STRING([--disable-elogger],
                   [Disable nanomsg event logger (some unit tests may fail)]))

AS_IF([test "x$enable_elogger" != "xno"], [
    AS_IF([test "$want_nanomsg" = "yes"], [
        elogger_enabled=yes
        AC_DEFINE([ELOG_ON], [], [Enable nanomsg event logger])
    ], [
        AC_MSG_WARN([Cannot use elogger without nanomsg])
    ])
])

AC_ARG_ENABLE([undeterministic_tests],
    AS_HELP_STRING([--disable-undeterministic-tests],
                   [Skip undeterministic tests (e.g. queueing) when running "make check"]))
AM_CONDITIONAL([SKIP_UNDETERMINISTIC_TESTS],
               [test "x$enable_undeterministic_tests" = "xno"])

AC_ARG_WITH([thrift],
    AS_HELP_STRING([--with-thrift], [Build Thrift RPC service, if disabled then you must have some other way of controlling the switch]),
    [want_thrift="$withval"], [want_thrift=yes])

AM_CONDITIONAL([COND_THRIFT], [test "$want_thrift" = yes])

want_pi=no
AC_ARG_WITH([pi],
    AS_HELP_STRING([--with-pi], [Build PI implementation for bmv2, this implementation is not the one included in the PI repository (which is the recommended one)]),
    [want_pi=yes], [])

AM_CONDITIONAL([COND_PI], [test "$want_pi" = yes])

AC_ARG_ENABLE([Werror],
    AS_HELP_STRING([--enable-Werror], [Make all compiler warnings fatal]),
    [enable_Werror="$enableval"], [enable_Werror=no])

AC_ARG_ENABLE([WP4-16-stacks],
    AS_HELP_STRING([--enable-WP4-16-stacks],
                   [Implement stacks strictly as per the P4_16 specification instead of legacy behavior]),
    [enable_WP4_16_stacks="$enableval"], [enable_WP4_16_stacks=yes])

AS_IF([test "$enable_WP4_16_stacks" = "yes"],
      [AC_DEFINE([WP4_16_STACKS], [], [Implement stacks as per P4_16 spec])])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
LT_INIT

AC_CONFIG_MACRO_DIR([m4])

# enforce -std=c++11
AX_CXX_COMPILE_STDCXX_11([noext],[mandatory])

# Checks for header files.
AC_LANG_PUSH(C)
AC_LANG_PUSH(C++)

# Thrift
AS_IF([test "$want_thrift" = no && test "$want_pdfixed" = yes],
    [AC_MSG_ERROR(cannot use --with-pdfixed if Thrift is disabled)])

AS_IF([test "$want_thrift" = yes], [
    AC_PATH_PROG([THRIFT], [thrift], [])
    AC_SUBST([THRIFT_LIB], ["-lthrift"])
    AC_CHECK_HEADER([thrift/Thrift.h], [], [AC_MSG_ERROR([Thrift headers not found. Install Thrift from http://thrift.apache.org/docs/install/])])
    AS_IF([test x"$THRIFT" = x], [AC_MSG_ERROR([cannot find thrift])])
    AC_DEFINE([THRIFT_ON], [], [Enable Thrift support])
    AC_CHECK_HEADER([thrift/stdcxx.h], [
        AC_DEFINE([HAVE_THRIFT_STDCXX_H], [], [Found Thrift stdcxx wrapper])
    ], [])

    AC_MSG_CHECKING(for thrift version)
    AC_RUN_IFELSE(
        [AC_LANG_SOURCE(
          [[#include <thrift/config.h>
            #include <stdio.h>
            int main() {
              int major, minor, revision;
              if (sscanf(PACKAGE_VERSION, "%d.%d.%d", &major, &minor, &revision) != 3)
                return 1;
              int version = major * 10000 + minor * 100 + revision;
              printf("%d\n", version);
              return 0;
            }
        ]])],
        [THRIFT_VERSION=`./conftest$EXEEXT`],
        [AC_MSG_RESULT(error)
         AC_MSG_ERROR(Cannot determine thrift version)]
    )
    AC_DEFINE_UNQUOTED([THRIFT_VERSION], [$THRIFT_VERSION],
                       [Thrift version string extracted from thrift/config.h])
])

AS_IF([test "$want_pi" = yes], [
    PI_url=https://github.com/p4lang/PI/
    AC_CHECK_HEADERS([PI/pi.h PI/target/pi_imp.h PI/p4info.h], [],
                     [AC_MSG_ERROR([Cannot find PI headers, did you install $PI_url])])
])

AC_CHECK_HEADERS([algorithm array cassert cmath queue \
cstdio string sys/stat.h sys/types.h ctime tuple unistd.h unordered_map \
utility vector], [], [AC_MSG_ERROR([Missing header file])])

AS_IF([test "$want_nanomsg" = yes], [
    AC_CHECK_LIB([nanomsg], [nn_errno], [], [AC_MSG_ERROR([Missing libnanomsg])])
    AC_DEFINE([NANOMSG_ON], [], [Enable Nanomsg support])
])

# Check for pthread, libjudy, libgmp, libpcap
AX_PTHREAD([], [AC_MSG_ERROR([Missing pthread library])])
AC_CHECK_LIB([Judy], [Judy1Next], [], [AC_MSG_ERROR([Missing libJudy])])
AC_CHECK_LIB([gmp], [__gmpz_init], [], [AC_MSG_ERROR([Missing libgmp])])
AC_CHECK_LIB([pcap], [pcap_create], [], [AC_MSG_ERROR([Missing libpcap])])
AC_CHECK_LIB([pcap], [pcap_set_immediate_mode], [pcap_fix=yes], [pcap_fix=no])
if test -n "$COVERAGE_FLAGS"; then
    AC_CHECK_LIB([gcov], [__gcov_init], [], [AC_MSG_ERROR([Missing gcov library])])
fi

AC_MSG_CHECKING(for compiler atomic support)
AC_LINK_IFELSE(
    [AC_LANG_SOURCE(
      [[#include <atomic>
        int main() {
          struct C { int x; int y; };
          std::atomic<C> c;
          C c1 = c.load();
          C c2;
          do { c2.x = c1.x + 1; c2.y = c1.y + 1; }
          while (!c.compare_exchange_weak(c1, c2));
          return 0;
        }
    ]])],
    [AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)
     AC_MSG_NOTICE([using -latomic])
     AX_CXX_CHECK_LIB([atomic], [__atomic_load_4], [], AC_MSG_ERROR([Missing latomic]))
    ])

AM_CONDITIONAL([WITH_PCAP_FIX], [test "$pcap_fix" = "yes"])

# Check for dlopen and dlfcn.h
AC_ARG_ENABLE(modules,
    AS_HELP_STRING([--enable-modules],
                   [Allow loading third-party modules at runtime]))
modules_enabled=no
AS_IF([test "x$enable_modules" != "xno"], [
  AC_MSG_CHECKING(for dlopen())
  AC_CHECK_HEADERS(dlfcn.h, [
    AC_SEARCH_LIBS([dlopen], [dl], [
      AC_DEFINE([HAVE_DLOPEN], [], [Found dlopen])
      AC_DEFINE([ENABLE_MODULES], [], [Enable dynamic loading of modules])
      modules_enabled=yes
    ], [
      AC_MSG_RESULT(no)
      AS_IF([test "x$enable_modules" == "xyes"], [
        AC_MSG_ERROR([Cannot enable modules without dlopen])
      ])
    ])
  ],[
    AC_MSG_RESULT(no)
    AS_IF([test "x$enable_modules" == "xyes"], [
      AC_MSG_ERROR([Cannot enable modules without dlfcn.h])
    ])
  ])
])
AM_CONDITIONAL([ENABLE_MODULES], [test "x$modules_enabled" == "xyes"])

# C++ libraries are harder (http://nerdland.net/2009/07/detecting-c-libraries-with-autotools/),
# so use headers to check
AC_CHECK_HEADER([boost/thread.hpp], [], [AC_MSG_ERROR([Boost threading headers not found])])
# need to check at least for the libboost_thread since it is -mt.so
AX_CXX_CHECK_LIB([boost_thread], [boost::thread], [],
                 [AX_CXX_CHECK_LIB([boost_thread-mt], [boost::thread],
                                   [LIBS="-lboost_thread-mt $LIBS"],
                                   [AC_MSG_ERROR([Missing boost thread library])])])
AC_CHECK_HEADER([boost/multiprecision/gmp.hpp], [], [AC_MSG_ERROR([Missing boost Multiprecision headers])])
AC_CHECK_HEADER([boost/program_options.hpp], [], [AC_MSG_ERROR([Missing boost program options header])])
AC_CHECK_HEADER([boost/functional/hash.hpp], [], [AC_MSG_ERROR([Missing boost functional hash header])])
AC_CHECK_HEADER([boost/filesystem.hpp], [], [AC_MSG_ERROR([Missing boost filesystem header])])

AC_SUBST([AM_CPPFLAGS], ["-I\$(top_srcdir)/include \
                          -I\$(top_builddir)/include \
                          -isystem\$(top_srcdir)/third_party/jsoncpp/include \
                          -isystem\$(top_srcdir)/third_party/spdlog"])
AC_SUBST([AM_CFLAGS], ["$PTHREAD_CFLAGS"])
# Using ax_append_compile_flags requires copying 4 macro definitions from the
# autoconf archive to m4/
MY_CXXFLAGS="-Wall -Wextra"
AS_IF([test "$enable_Werror" = "yes"], [MY_CXXFLAGS="$MY_CXXFLAGS -Werror"])
AC_SUBST([AM_CXXFLAGS], ["$MY_CXXFLAGS $PTHREAD_CFLAGS"])

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
                include/Makefile
                src/Makefile
                src/bf_lpm_trie/Makefile
		src/bm_sim/Makefile
		src/bm_runtime/Makefile
		src/BMI/Makefile
		src/bm_apps/Makefile
		src/bm_apps/examples/Makefile
		targets/Makefile
		targets/simple_router/Makefile
		targets/l2_switch/Makefile
		targets/l2_switch/learn_client/Makefile
		targets/simple_switch/Makefile
		targets/simple_switch/tests/Makefile
		targets/simple_switch/tests/CLI_tests/Makefile
		targets/psa_switch/Makefile
		targets/psa_switch/tests/Makefile
		tests/Makefile
		tests/stress_tests/Makefile
                tools/Makefile
                pdfixed/Makefile
                pdfixed/include/Makefile
                PI/Makefile])

# Generate other files
AC_CONFIG_FILES([tests/utils.cpp
                 src/bm_sim/version.cpp
                 mininet/stress_test_ipv4.py])
AC_CONFIG_FILES([targets/simple_switch/tests/CLI_tests/run_one_test.py],
                [chmod +x targets/simple_switch/tests/CLI_tests/run_one_test.py])

AX_PREFIX_CONFIG_H([include/bm/config.h], [BM])

AC_OUTPUT

AS_ECHO("")
AS_ECHO("Features recap ......................")
AS_ECHO("Coverage enabled .............. : $coverage_enabled")
AS_ECHO("Logging macros enabled ........ : $logging_macros_enabled")
AS_ECHO("With Nanomsg .................. : $want_nanomsg")
AS_ECHO("Event logger enabled .......... : $elogger_enabled")
AS_ECHO("Debugger enabled .............. : $debugger_enabled")
AS_ECHO("With Thrift ................... : $want_thrift")
AS_ECHO("With pdfixed .................. : $want_pdfixed")
AS_ECHO("With PI ....................... : $want_pi")
