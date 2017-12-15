ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

libbmpi_la_SOURCES = \
src/pi.cpp \
src/pi_imp.cpp \
src/pi_tables_imp.cpp \
src/pi_act_prof_imp.cpp \
src/pi_counter_imp.cpp \
src/pi_meter_imp.cpp \
src/pi_learn_imp.cpp \
src/common.h \
src/common.cpp \
src/action_helpers.h \
src/action_helpers.cpp \
src/direct_res_spec.h \
src/direct_res_spec.cpp

libbmpi_la_LIBADD = \
$(top_builddir)/src/bm_sim/libbmsim.la

lib_LTLIBRARIES = libbmpi.la

nobase_include_HEADERS = bm/PI/pi.h
