AC_PREREQ(2.64)
AC_INIT([p4c], [0.2], [cdodd@barefootnetworks.com])
m4_include([m4/ax_cxx_compile_stdcxx.m4])

AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE([foreign subdir-objects 1.14])
AM_SILENT_RULES([yes])

LT_INIT
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LEX
AC_PROG_YACC
AX_CXX_COMPILE_STDCXX_11([noext],[mandatory])

AC_CHECK_FUNCS([clock_gettime])
AC_CHECK_FUNCS([memchr])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([strchr])
AC_CHECK_FUNCS([strcspn])
AC_CHECK_FUNCS([strrchr])
AC_CHECK_FUNCS([strstr])
AC_CHECK_FUNCS([strtol])
AC_CHECK_HEADERS([libintl.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([execinfo.h])
AC_CHECK_HEADERS([ucontext.h])
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_FUNC_ALLOCA
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

AC_CHECK_HEADERS([constraint_solver/constraint_solver.h])
AC_CHECK_LIB([gc], [GC_malloc], [], [AC_MSG_ERROR([Missing GC library])])
AC_CHECK_LIB([rt], [clock_gettime], [], [])
AC_CHECK_LIB([gmp], [__gmpz_init], [], [AC_MSG_ERROR([GNU MP not found])])
AC_CHECK_LIB([gmpxx], [__gmpz_init], [], [AC_MSG_ERROR([GNU MP not found])])

AC_SUBST(P4INCLUDE, $prefix/p4include)

AC_CONFIG_FILES([Makefile setup.h])
AC_OUTPUT
