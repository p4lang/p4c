AM_CPPFLAGS = -I$(srcdir)/include/
AM_CXXFLAGS = $(PICKY_CXXFLAGS)
AM_CFLAGS = $(PICKY_CFLAGS)

noinst_LTLIBRARIES = libbmi.la

common_source = \
src/bmi_interface.c \
src/bmi_interface.h \
src/bmi_port.c

common_include = \
include/BMI/bmi_port.h

libbmi_la_SOURCES = $(common_source) $(common_include)
