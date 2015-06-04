BUILT_SOURCES = $(srcdir)/../tools/runtime/__init__.py gen-cpp/runtime_constants.cpp gen-cpp/runtime_constants.h gen-cpp/Runtime.cpp gen-cpp/Runtime.h gen-cpp/runtime_types.cpp gen-cpp/runtime_types.h

AUTOMAKE_OPTIONS = foreign no-dependencies subdir-objects

noinst_LTLIBRARIES = libruntimestubs.la
libruntimestubs_la_SOURCES = \
	${BUILT_SOURCES}

${BUILT_SOURCES} : runtime.thrift
	$(THRIFT) --gen cpp -r $<
	$(THRIFT) --gen py -r $<
	rm -rf $(srcdir)/../tools/runtime/
	cp -r gen-py/runtime/ $(srcdir)/../tools/

clean-local:
	rm -rf gen-cpp gen-py
