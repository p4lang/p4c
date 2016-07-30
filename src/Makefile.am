ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

SUBDIRS = bf_lpm_trie BMI bm_sim bm_runtime bm_apps

lib_LTLIBRARIES = libbmp4apps.la libbmall.la

libbmp4apps_la_SOURCES =
# Dummy C++ source to cause C++ linking.
nodist_EXTRA_libbmp4apps_la_SOURCES = dummy.cpp
libbmp4apps_la_LIBADD = bm_apps/libbmapps.la

libbmall_la_SOURCES =
# Dummy C++ source to cause C++ linking.
nodist_EXTRA_libbmall_la_SOURCES = dummy.cpp
libbmall_la_LIBADD = \
$(top_builddir)/src/bm_sim/libbmsim.la \
$(top_builddir)/src/bf_lpm_trie/libbflpmtrie.la \
$(top_builddir)/src/BMI/libbmi.la \
$(top_builddir)/third_party/jsoncpp/libjson.la \
$(top_builddir)/src/bm_runtime/libbmruntime.la \
$(top_builddir)/thrift_src/libruntimestubs.la \
-lboost_system -lboost_thread $(THRIFT_LIB) -lboost_program_options

libbmall_la_LDFLAGS = -pthread
