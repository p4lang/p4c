standard_thrift_files = \
$(srcdir)/../tools/standard/__init__.py \
gen-cpp/standard_constants.cpp \
gen-cpp/standard_constants.h \
gen-cpp/Standard.cpp \
gen-cpp/Standard.h \
gen-cpp/standard_types.cpp \
gen-cpp/standard_types.h

simple_pre_thrift_files = \
$(srcdir)/../tools/simple_pre/__init__.py \
gen-cpp/simple_pre_constants.cpp \
gen-cpp/simple_pre_constants.h \
gen-cpp/SimplePre.cpp \
gen-cpp/SimplePre.h \
gen-cpp/simple_pre_types.cpp \
gen-cpp/simple_pre_types.h

BUILT_SOURCES = \
$(standard_thrift_files) \
$(simple_pre_thrift_files)

AUTOMAKE_OPTIONS = foreign no-dependencies subdir-objects

noinst_LTLIBRARIES = libruntimestubs.la
libruntimestubs_la_SOURCES = \
	$(BUILT_SOURCES)

$(standard_thrift_files) : standard.thrift
	$(THRIFT) --gen cpp -r $<
	$(THRIFT) --gen py -r $<
	rm -rf $(srcdir)/../tools/bm_runtime/standard/
	mkdir -p $(srcdir)/../tools/bm_runtime
	cp -r gen-py/bm_runtime/standard/ $(srcdir)/../tools/bm_runtime/

$(simple_pre_thrift_files) : simple_pre.thrift
	$(THRIFT) --gen cpp -r $<
	$(THRIFT) --gen py -r $<
	rm -rf $(srcdir)/../tools/bm_runtime/simple_pre/
	mkdir -p $(srcdir)/../tools/bm_runtime
	cp -r gen-py/bm_runtime/simple_pre/ $(srcdir)/../tools/bm_runtime/

clean-local:
	rm -rf gen-cpp gen-py
