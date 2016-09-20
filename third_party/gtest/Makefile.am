AM_CPPFLAGS = -I$(srcdir)/include
AM_CXXFLAGS =

check_LTLIBRARIES = libgtest.la

EXTRA_DIST = \
include/gtest/gtest-printers.h \
include/gtest/gtest-test-part.h \
include/gtest/internal/custom/gtest-printers.h \
include/gtest/internal/custom/gtest.h \
include/gtest/internal/custom/gtest-port.h \
include/gtest/internal/gtest-internal.h \
include/gtest/internal/gtest-port-arch.h \
include/gtest/internal/gtest-port.h \
include/gtest/internal/gtest-type-util.h \
include/gtest/internal/gtest-tuple.h \
include/gtest/internal/gtest-linked_ptr.h \
include/gtest/internal/gtest-string.h \
include/gtest/internal/gtest-filepath.h \
include/gtest/internal/gtest-param-util.h \
include/gtest/internal/gtest-death-test-internal.h \
include/gtest/internal/gtest-param-util-generated.h \
include/gtest/gtest-param-test.h \
include/gtest/gtest-typed-test.h \
include/gtest/gtest-message.h \
include/gtest/gtest_prod.h \
include/gtest/gtest_pred_impl.h \
include/gtest/gtest-spi.h \
include/gtest/gtest-death-test.h

EXTRA_DIST += \
src/gtest-test-part.cc \
src/gtest-filepath.cc \
src/gtest-typed-test.cc \
src/gtest-port.cc \
src/gtest_main.cc \
src/gtest-death-test.cc \
src/gtest.cc \
src/gtest-printers.cc \
src/gtest-internal-inl.h

libgtest_la_SOURCES = \
src/gtest-all.cc \
include/gtest/gtest.h
