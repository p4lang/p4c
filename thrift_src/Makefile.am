standard_thrift_py_files = \
gen-py/bm_runtime/standard/constants.py \
gen-py/bm_runtime/standard/__init__.py \
gen-py/bm_runtime/standard/Standard.py \
gen-py/bm_runtime/standard/ttypes.py
standard_thrift_files = \
gen-cpp/bm/standard_constants.cpp \
gen-cpp/bm/standard_constants.h \
gen-cpp/bm/Standard.cpp \
gen-cpp/bm/Standard.h \
gen-cpp/bm/standard_types.cpp \
gen-cpp/bm/standard_types.h

simple_pre_thrift_py_files = \
gen-py/bm_runtime/simple_pre/constants.py \
gen-py/bm_runtime/simple_pre/__init__.py \
gen-py/bm_runtime/simple_pre/SimplePre.py \
gen-py/bm_runtime/simple_pre/ttypes.py
simple_pre_thrift_files = \
gen-cpp/bm/simple_pre_constants.cpp \
gen-cpp/bm/simple_pre_constants.h \
gen-cpp/bm/SimplePre.cpp \
gen-cpp/bm/SimplePre.h \
gen-cpp/bm/simple_pre_types.cpp \
gen-cpp/bm/simple_pre_types.h

simple_pre_lag_thrift_py_files = \
gen-py/bm_runtime/simple_pre_lag/constants.py \
gen-py/bm_runtime/simple_pre_lag/__init__.py \
gen-py/bm_runtime/simple_pre_lag/SimplePreLAG.py \
gen-py/bm_runtime/simple_pre_lag/ttypes.py
simple_pre_lag_thrift_files = \
gen-cpp/bm/simple_pre_lag_constants.cpp \
gen-cpp/bm/simple_pre_lag_constants.h \
gen-cpp/bm/SimplePreLAG.cpp \
gen-cpp/bm/SimplePreLAG.h \
gen-cpp/bm/simple_pre_lag_types.cpp \
gen-cpp/bm/simple_pre_lag_types.h

thrift_cxx_files = \
$(standard_thrift_files) \
$(simple_pre_thrift_files) \
$(simple_pre_lag_thrift_files)

BUILT_SOURCES = \
$(thrift_cxx_files) \
$(standard_thrift_py_files) \
$(simple_pre_thrift_py_files) \
$(simple_pre_lag_thrift_py_files) \
gen-py/bm_runtime/__init__.py

rootpydir = $(pythondir)/bm_runtime/
nodist_rootpy_PYTHON = gen-py/bm_runtime/__init__.py

standardpydir = $(pythondir)/bm_runtime/standard
nodist_standardpy_PYTHON = $(standard_thrift_py_files)

simple_prepydir = $(pythondir)/bm_runtime/simple_pre
nodist_simple_prepy_PYTHON = $(simple_pre_thrift_py_files)

simple_pre_lagpydir = $(pythondir)/bm_runtime/simple_pre_lag
nodist_simple_pre_lagpy_PYTHON = $(simple_pre_lag_thrift_py_files)

AUTOMAKE_OPTIONS = foreign no-dependencies subdir-objects

lib_LTLIBRARIES = libruntimestubs.la

nodist_libruntimestubs_la_SOURCES = \
$(thrift_cxx_files)

nodist_pkginclude_HEADERS = \
gen-cpp/bm/standard_constants.h \
gen-cpp/bm/Standard.h \
gen-cpp/bm/standard_types.h \
gen-cpp/bm/simple_pre_constants.h \
gen-cpp/bm/SimplePre.h \
gen-cpp/bm/simple_pre_types.h \
gen-cpp/bm/simple_pre_lag_constants.h \
gen-cpp/bm/SimplePreLAG.h \
gen-cpp/bm/simple_pre_lag_types.h

# See http://www.gnu.org/software/automake/manual/html_node/Multiple-Outputs.html

# We copy the generated Python code to tools/ if we have permissions (i.e. not
# for 'make distcheck'). This is to ensure we can run the Python scripts in
# tools easily. Maybe I will think of a better solution later.
thrift_files.ts: $(srcdir)/standard.thrift $(srcdir)/simple_pre.thrift $(srcdir)/simple_pre_lag.thrift
	@rm -f thrift_files.tmp
	@touch thrift_files.tmp
	@mkdir -p $(builddir)/gen-cpp/bm
	$(THRIFT) -out $(builddir)/gen-cpp/bm --gen cpp -r $(srcdir)/standard.thrift
	$(THRIFT) -o $(builddir) --gen py -r $(srcdir)/standard.thrift
	$(THRIFT) -out $(builddir)/gen-cpp/bm --gen cpp -r $(srcdir)/simple_pre.thrift
	$(THRIFT) -o $(builddir) --gen py -r $(srcdir)/simple_pre.thrift
	$(THRIFT) -out $(builddir)/gen-cpp/bm --gen cpp -r $(srcdir)/simple_pre_lag.thrift
	$(THRIFT) -o $(builddir) --gen py -r $(srcdir)/simple_pre_lag.thrift
	if mkdir $(top_srcdir)/tools/bm_runtime.test 2>/dev/null; then \
	  rm -rf $(top_srcdir)/tools/bm_runtime/; \
	  cp -r $(builddir)/gen-py/bm_runtime/ $(top_srcdir)/tools/; \
	  rm -rf $(top_srcdir)/tools/bm_runtime.test; else :; \
        fi
	@mv -f thrift_files.tmp $@

EXTRA_DIST = \
$(srcdir)/standard.thrift \
$(srcdir)/simple_pre.thrift \
$(srcdir)/simple_pre_lag.thrift

$(BUILT_SOURCES): thrift_files.ts
## Recover from the removal of $@
	@if test -f $@; then :; else \
	  trap 'rm -rf thrift_files.lock thrift_files.ts' 1 2 13 15; \
## mkdir is a portable test-and-set
	if mkdir thrift_files.lock 2>/dev/null; then \
## This code is being executed by the first process.
	  rm -f thrift_files.ts; \
	  $(MAKE) $(AM_MAKEFLAGS) thrift_files.ts; \
	  result=$$?; rm -rf thrift_files.lock; exit $$result; \
	else \
## This code is being executed by the follower processes.
## Wait until the first process is done.
	  while test -d thrift_files.lock; do sleep 1; done; \
## Succeed if and only if the first process succeeded.
	    test -f thrift_files.ts; \
	  fi; \
	fi


CLEANFILES = $(BUILT_SOURCES) \
thrift_files.ts

clean-local:
	rm -rf gen-cpp gen-py
