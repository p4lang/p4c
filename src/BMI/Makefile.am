ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

if WITH_PCAP_FIX
AM_CPPFLAGS += -DWITH_PCAP_FIX
endif

noinst_LTLIBRARIES = libbmi.la

libbmi_la_SOURCES = \
bmi_interface.c \
bmi_interface.h \
bmi_port.c \
BMI/bmi_port.h
