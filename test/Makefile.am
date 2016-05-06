# Unit tests

check_PROGRAMS = exception_test format_test source_file_test path_test \
		 enumerator_test default_test unittest_transform1 json_test

default_test_SOURCES = test/unittests/default_test.cpp
default_test_LDADD = libp4ctoolkit.a
exception_test_SOURCES = test/unittests/exception_test.cpp
exception_test_LDADD = libp4ctoolkit.a
enumerator_test_SOURCES = test/unittests/enumerator_test.cpp
enumerator_test_LDADD = libp4ctoolkit.a
source_file_test_SOURCES = test/unittests/source_file_test.cpp
source_file_test_LDADD = libp4ctoolkit.a
format_test_SOURCES = test/unittests/format_test.cpp
format_test_LDADD = libp4ctoolkit.a
path_test_SOURCES = test/unittests/path_test.cpp
path_test_LDADD = libp4ctoolkit.a
json_test_SOURCES = test/unittests/json_test.cpp
json_test_LDADD = libp4ctoolkit.a
unittest_transform1_SOURCES = $(ir_SOURCES) test/unittests/transform1.cpp
unittest_transform1_LDADD = libfrontend.a libp4ctoolkit.a

# Compiler tests

TESTS += $(check_PROGRAMS)
