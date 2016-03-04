SUBDIRS = bf_lpm_trie BMI bm_sim bm_runtime bm_apps

lib_LTLIBRARIES = libbmp4apps.la

libbmp4apps_la_SOURCES =
# Dummy C++ source to cause C++ linking.
nodist_EXTRA_libbmp4apps_la_SOURCES = dummy.cpp
libbmp4apps_la_LIBADD = bm_apps/libbmapps.la
