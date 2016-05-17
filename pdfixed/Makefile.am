ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

SUBDIRS = include

pd_conn_mgr_thrift_cxx = \
pd_thrift_gen/gen-cpp/conn_mgr.cpp \
pd_thrift_gen/gen-cpp/conn_mgr.h \
pd_thrift_gen/gen-cpp/conn_mgr_pd_rpc_constants.cpp \
pd_thrift_gen/gen-cpp/conn_mgr_pd_rpc_constants.h \
pd_thrift_gen/gen-cpp/conn_mgr_pd_rpc_types.cpp \
pd_thrift_gen/gen-cpp/conn_mgr_pd_rpc_types.h
pd_conn_mgr_thrift_py = \
pd_thrift_gen/gen-py/conn_mgr_pd_rpc/constants.py \
pd_thrift_gen/gen-py/conn_mgr_pd_rpc/__init__.py \
pd_thrift_gen/gen-py/conn_mgr_pd_rpc/ttypes.py \
pd_thrift_gen/gen-py/conn_mgr_pd_rpc/conn_mgr.py \
pd_thrift_gen/gen-py/conn_mgr_pd_rpc/conn_mgr-remote
connmgrpkgpythondir = $(pkgpythondir)/pdfixed/conn_mgr_pd_rpc
nodist_connmgrpkgpython_PYTHON = $(pd_conn_mgr_thrift_py)

pd_mc_thrift_cxx = \
pd_thrift_gen/gen-cpp/mc.cpp \
pd_thrift_gen/gen-cpp/mc.h \
pd_thrift_gen/gen-cpp/mc_pd_rpc_constants.cpp \
pd_thrift_gen/gen-cpp/mc_pd_rpc_constants.h \
pd_thrift_gen/gen-cpp/mc_pd_rpc_types.cpp \
pd_thrift_gen/gen-cpp/mc_pd_rpc_types.h
pd_mc_thrift_py = \
pd_thrift_gen/gen-py/mc_pd_rpc/constants.py \
pd_thrift_gen/gen-py/mc_pd_rpc/__init__.py \
pd_thrift_gen/gen-py/mc_pd_rpc/ttypes.py \
pd_thrift_gen/gen-py/mc_pd_rpc/mc.py \
pd_thrift_gen/gen-py/mc_pd_rpc/mc-remote
mcpkgpythondir = $(pkgpythondir)/pdfixed/mc_pd_rpc
nodist_mcpkgpython_PYTHON = $(pd_mc_thrift_py)

pd_sswitch_thrift_cxx = \
pd_thrift_gen/gen-cpp/sswitch.cpp \
pd_thrift_gen/gen-cpp/sswitch.h \
pd_thrift_gen/gen-cpp/sswitch_pd_rpc_constants.cpp \
pd_thrift_gen/gen-cpp/sswitch_pd_rpc_constants.h \
pd_thrift_gen/gen-cpp/sswitch_pd_rpc_types.cpp \
pd_thrift_gen/gen-cpp/sswitch_pd_rpc_types.h
pd_sswitch_thrift_py = \
pd_thrift_gen/gen-py/sswitch_pd_rpc/constants.py \
pd_thrift_gen/gen-py/sswitch_pd_rpc/__init__.py \
pd_thrift_gen/gen-py/sswitch_pd_rpc/ttypes.py \
pd_thrift_gen/gen-py/sswitch_pd_rpc/sswitch.py \
pd_thrift_gen/gen-py/sswitch_pd_rpc/sswitch-remote
sswitchpkgpythondir = $(pkgpythondir)/pdfixed/sswitch_pd_rpc
nodist_sswitchpkgpython_PYTHON = $(pd_sswitch_thrift_py)

pd_res_thrift_cxx = \
pd_thrift_gen/gen-cpp/res_constants.cpp \
pd_thrift_gen/gen-cpp/res_constants.h \
pd_thrift_gen/gen-cpp/res_types.cpp \
pd_thrift_gen/gen-cpp/res_types.h
pd_res_thrift_py = \
pd_thrift_gen/gen-py/res_pd_rpc/constants.py \
pd_thrift_gen/gen-py/res_pd_rpc/__init__.py \
pd_thrift_gen/gen-py/res_pd_rpc/ttypes.py
respkgpythondir = $(pkgpythondir)/pdfixed/res_pd_rpc
nodist_respkgpython_PYTHON = $(pd_res_thrift_py)

pd_conn_mgr_IDL = $(srcdir)/thrift/conn_mgr_pd_rpc.thrift
pd_mc_IDL = $(srcdir)/thrift/mc_pd_rpc.thrift
pd_sswitch_IDL = $(srcdir)/thrift/sswitch_pd_rpc.thrift
pd_res_IDL = $(srcdir)/thrift/res.thrift

EXTRA_DIST = $(pd_conn_mgr_IDL) $(pd_mc_IDL) $(pd_sswitch_IDL) $(pd_res_IDL)

pdfixed_thrift_files_cxx = \
$(pd_conn_mgr_thrift_cxx) \
$(pd_mc_thrift_cxx) \
$(pd_sswitch_thrift_cxx) \
$(pd_res_thrift_cxx)

pdfixed_thrift_files = \
$(pdfixed_thrift_files_cxx) \
$(pd_conn_mgr_thrift_py) \
$(pd_mc_thrift_py) \
$(pd_sswitch_thrift_py) \
$(pd_res_thrift_py)

BUILT_SOURCES = $(pdfixed_thrift_files) \
pd_thrift_gen/gen-py/__init__.py

pdfixed_thrift_files.ts: $(pd_conn_mgr_IDL) $(pd_mc_IDL) $(pd_sswitch_IDL) $(pd_res_IDL)
	@rm -f pdfixed_thrift_files.tmp
	@touch pdfixed_thrift_files.tmp
	@mkdir -p $(builddir)/pd_thrift_gen
	$(THRIFT) -o $(builddir)/pd_thrift_gen/ --gen cpp -r $(pd_conn_mgr_IDL)
	$(THRIFT) -o $(builddir)/pd_thrift_gen/ --gen cpp -r $(pd_mc_IDL)
	$(THRIFT) -o $(builddir)/pd_thrift_gen/ --gen cpp -r $(pd_sswitch_IDL)
	$(THRIFT) -o $(builddir)/pd_thrift_gen/ --gen py -r $(pd_conn_mgr_IDL)
	$(THRIFT) -o $(builddir)/pd_thrift_gen/ --gen py -r $(pd_mc_IDL)
	$(THRIFT) -o $(builddir)/pd_thrift_gen/ --gen py -r $(pd_sswitch_IDL)
	@mv -f pdfixed_thrift_files.tmp $@
$(BUILT_SOURCES): pdfixed_thrift_files.ts
## Recover from the removal of $@
	@if test -f $@; then :; else \
	  trap 'rm -rf pdfixed_thrift_files.lock pdfixed_thrift_files.ts' 1 2 13 15; \
## mkdir is a portable test-and-set
	if mkdir pdfixed_thrift_files.lock 2>/dev/null; then \
## This code is being executed by the first process.
	  rm -f pdfixed_thrift_files.ts; \
	  $(MAKE) $(AM_MAKEFLAGS) pdfixed_thrift_files.ts; \
	  result=$$?; rm -rf pdfixed_thrift_files.lock; exit $$result; \
	else \
## This code is being executed by the follower processes.
## Wait until the first process is done.
	  while test -d pdfixed_thrift_files.lock; do sleep 1; done; \
## Succeed if and only if the first process succeeded.
	    test -f pdfixed_thrift_files.ts; \
	  fi; \
	fi

pdf_files = \
src/pd_notifications.cpp \
src/pd_helpers.cpp \
src/pd_pre.cpp \
src/pd_mirroring.cpp \
src/pd_static.cpp \
src/pd_swap.cpp

pdfixedpkgincludedir = $(pkgincludedir)/pdfixed

nobase_pdfixedpkginclude_HEADERS = \
thrift-src/pdfixed_rpc_server.h \
thrift/res.thrift

lib_LTLIBRARIES = libbmpdfixed.la libbmpdfixedthrift.la

libbmpdfixed_la_SOURCES = \
$(pdf_files)

libbmpdfixed_la_LIBADD = \
$(top_builddir)/thrift_src/libruntimestubs.la \
$(top_builddir)/targets/simple_switch/libsimpleswitch_thrift.la

libbmpdfixedthrift_la_SOURCES = \
thrift-src/pdfixed_rpc_server.cpp \
thrift-src/conn_mgr_pd_rpc_server.ipp \
thrift-src/mc_pd_rpc_server.ipp \
thrift-src/sswitch_pd_rpc_server.ipp

nodist_libbmpdfixedthrift_la_SOURCES = \
$(pdfixed_thrift_files_cxx)

AM_CXXFLAGS =
AM_CPPFLAGS = -I$(top_builddir)/thrift_src/gen-cpp
AM_CPPFLAGS += -I$(top_builddir)/targets/simple_switch/gen-cpp
AM_CPPFLAGS += -I$(srcdir)/include
AM_CPPFLAGS += -I$(top_srcdir)/include
AM_CPPFLAGS += -I$(builddir)/pd_thrift_gen/gen-cpp/

if P4THRIFT
    AM_CPPFLAGS += -DP4THRIFT
endif

CLEANFILES = $(BUILT_SOURCES) \
pdfixed_thrift_files.ts \
pd_thrift_gen/gen-cpp/mc_server.skeleton.cpp \
pd_thrift_gen/gen-cpp/sswitch_server.skeleton.cpp \
pd_thrift_gen/gen-cpp/conn_mgr_server.skeleton.cpp
