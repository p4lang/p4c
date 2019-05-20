ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

if COND_TARGETS
AM_CPPFLAGS += -I$(top_srcdir)/targets/simple_switch -DWITH_SIMPLE_SWITCH
endif

libbmpi_la_SOURCES = \
src/pi.cpp \
src/pi_imp.cpp \
src/pi_tables_imp.cpp \
src/pi_act_prof_imp.cpp \
src/pi_counter_imp.cpp \
src/pi_meter_imp.cpp \
src/pi_learn_imp.cpp \
src/pi_mc_imp.cpp \
src/pi_clone_imp.cpp \
src/common.h \
src/common.cpp \
src/action_helpers.h \
src/action_helpers.cpp \
src/direct_res_spec.h \
src/direct_res_spec.cpp \
src/group_selection.h \
src/group_selection.cpp \
src/device_state.h \
src/device_state.cpp

lib_LTLIBRARIES = libbmpi.la

nobase_include_HEADERS = bm/PI/pi.h
