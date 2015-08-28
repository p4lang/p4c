standard_thrift_files = \
$(srcdir)/../tools/bm_runtime/standard/__init__.py \
gen-cpp/standard_constants.cpp \
gen-cpp/standard_constants.h \
gen-cpp/Standard.cpp \
gen-cpp/Standard.h \
gen-cpp/standard_types.cpp \
gen-cpp/standard_types.h

simple_pre_thrift_files = \
$(srcdir)/../tools/bm_runtime/simple_pre/__init__.py \
gen-cpp/simple_pre_constants.cpp \
gen-cpp/simple_pre_constants.h \
gen-cpp/SimplePre.cpp \
gen-cpp/SimplePre.h \
gen-cpp/simple_pre_types.cpp \
gen-cpp/simple_pre_types.h

simple_pre_lag_thrift_files = \
$(srcdir)/../tools/bm_runtime/simple_pre_lag/__init__.py \
gen-cpp/simple_pre_lag_constants.cpp \
gen-cpp/simple_pre_lag_constants.h \
gen-cpp/SimplePreLAG.cpp \
gen-cpp/SimplePreLAG.h \
gen-cpp/simple_pre_lag_types.cpp \
gen-cpp/simple_pre_lag_types.h

BUILT_SOURCES = \
$(standard_thrift_files) \
$(simple_pre_thrift_files) \
$(simple_pre_lag_thrift_files)

AUTOMAKE_OPTIONS = foreign no-dependencies subdir-objects

noinst_LTLIBRARIES = libruntimestubs.la
libruntimestubs_la_SOURCES = \
	$(BUILT_SOURCES)


# See http://www.gnu.org/software/automake/manual/html_node/Multiple-Outputs.html

thrift_files.ts: standard.thrift simple_pre.thrift simple_pre_lag.thrift
	@rm -f thrift_files.tmp
	@touch thrift_files.tmp
	$(THRIFT) --gen cpp -r standard.thrift
	$(THRIFT) --gen py -r standard.thrift
	$(THRIFT) --gen cpp -r simple_pre.thrift
	$(THRIFT) --gen py -r simple_pre.thrift
	$(THRIFT) --gen cpp -r simple_pre_lag.thrift
	$(THRIFT) --gen py -r simple_pre_lag.thrift
	rm -rf $(srcdir)/../tools/bm_runtime/
	cp -r gen-py/bm_runtime/ $(srcdir)/../tools/
	@mv -f thrift_files.tmp $@

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
