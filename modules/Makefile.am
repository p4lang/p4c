SUBDIRS = bf_lpm_trie BMI bm_sim bm_runtime bm_apps

lib_LTLIBRARIES = libp4.la libp4runtime.la libbmi.la libp4apps.la

libp4_la_SOURCES =
libp4_la_LIBADD = bf_lpm_trie/libbflpmtrie.la bm_sim/libbmsim.la

libp4runtime_la_SOURCES =
libp4runtime_la_LIBADD = bm_runtime/libbmruntime.la ../thrift_src/libruntimestubs.la

libbmi_la_SOURCES =
libbmi_la_LIBADD = BMI/libbmi.la

libp4apps_la_SOURCES =
libp4apps_la_LIBADD = bm_apps/libbmapps.la
