AM_CXXFLAGS = $(PICKY_CXXFLAGS)
AM_CFLAGS = $(PICKY_CFLAGS)

noinst_LIBRARIES = libbmruntime.a

common_source = \
src/runtime_constants.cpp \
src/runtime_constants.h \
src/Runtime.cpp \
src/Runtime.h \
src/Runtime_server.cpp \
src/runtime_types.cpp \
src/runtime_types.h

common_include = \
include/bm_runtime/bm_runtime.h

libbmruntime_a_SOURCES = $(common_source) $(common_include)
