ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

SUBDIRS = . tests

AM_CPPFLAGS += \
-I$(srcdir)/../../include \
-I$(srcdir)/../simple_switch \
-I$(srcdir)/../../PI \
-isystem$(srcdir)/../../third_party/spdlog

if WITH_SYSREPO
AM_CPPFLAGS += -DWITH_SYSREPO
endif

if WITH_THRIFT
AM_CPPFLAGS += -DWITH_THRIFT \
-I$(builddir)/../simple_switch/gen-cpp
endif

bin_PROGRAMS = simple_switch_grpc

simple_switch_grpc_SOURCES = main.cpp

simple_switch_grpc_LDADD = \
libsimple_switch_grpc.la

# We follow this tutorial to link with grpc++_reflection:
# https://github.com/grpc/grpc/blob/master/doc/server_reflection_tutorial.md
simple_switch_grpc_LDFLAGS = \
-Wl,--no-as-needed,-lgrpc++_reflection,--as-needed \
-rdynamic

lib_LTLIBRARIES = libbm_grpc_dataplane.la
noinst_LTLIBRARIES = libsimple_switch_grpc.la

libsimple_switch_grpc_la_SOURCES = \
switch_runner.cpp switch_runner.h
if WITH_SYSREPO
libsimple_switch_grpc_la_SOURCES += \
switch_sysrepo.h switch_sysrepo.cpp
endif

libsimple_switch_grpc_la_LIBADD = \
$(builddir)/../simple_switch/libsimpleswitch.la \
$(builddir)/../../PI/libbmpi.la \
libbm_grpc_dataplane.la

if WITH_THRIFT
libsimple_switch_grpc_la_LIBADD += \
$(builddir)/../../src/bm_runtime/libbmruntime.la \
$(builddir)/../../thrift_src/libruntimestubs.la \
$(builddir)/../simple_switch/libsimpleswitch_thrift.la
endif

libsimple_switch_grpc_la_LIBADD += \
-lpifeproto -lpigrpcserver -lpi -lpip4info \
$(GRPC_LIBS) $(PROTOBUF_LIBS)

# dataplane_interface.proto

EXTRA_DIST = p4/bm/dataplane_interface.proto

proto = $(abs_srcdir)/p4/bm/dataplane_interface.proto

PROTOFLAGS = -I$(abs_srcdir)

proto_cpp_files = \
cpp_out/p4/bm/dataplane_interface.pb.cc \
cpp_out/p4/bm/dataplane_interface.pb.h

proto_grpc_files = \
grpc_out/p4/bm/dataplane_interface.grpc.pb.cc \
grpc_out/p4/bm/dataplane_interface.grpc.pb.h

includep4bmdir = $(includedir)/p4/bm/
nodist_includep4bm_HEADERS = \
cpp_out/p4/bm/dataplane_interface.pb.h \
grpc_out/p4/bm/dataplane_interface.grpc.pb.h

BUILT_SOURCES = $(proto_cpp_files) $(proto_grpc_files)

if HAVE_GRPC_PY_PLUGIN
p4bmpydir = $(pythondir)/p4/bm
nodist_p4bmpy_PYTHON = \
py_out/p4/bm/dataplane_interface_pb2.py \
py_out/p4/bm/__init__.py

BUILT_SOURCES += \
$(nodist_p4bmpy_PYTHON)
endif

# See http://www.gnu.org/software/automake/manual/html_node/Multiple-Outputs.html

proto_files.ts: $(proto)
	@rm -f proto_files.tmp
	@touch proto_files.tmp
	@mkdir -p $(builddir)/cpp_out
	@mkdir -p $(builddir)/grpc_out
	$(PROTOC) $^ --cpp_out $(builddir)/cpp_out $(PROTOFLAGS)
	$(PROTOC) $^ --grpc_out $(builddir)/grpc_out --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) $(PROTOFLAGS)
if HAVE_GRPC_PY_PLUGIN
	@mkdir -p $(builddir)/py_out
# With the Python plugin, it seems that I need to use a single command for proto
# + grpc and that the output directory needs to be the same (because the grpc
# plugin inserts code into the proto-generated files). But maybe I am just using
# an old version of the Python plugin.
	$(PROTOC) $^ --python_out $(builddir)/py_out $(PROTOFLAGS) --grpc_out $(builddir)/py_out --plugin=protoc-gen-grpc=$(GRPC_PY_PLUGIN)
	@touch $(builddir)/py_out/p4/bm/__init__.py
endif
	@mv -f proto_files.tmp $@

$(BUILT_SOURCES): proto_files.ts
## Recover from the removal of $@
	@if test -f $@; then :; else \
	  trap 'rm -rf proto_files.lock proto_files.ts' 1 2 13 15; \
## mkdir is a portable test-and-set
	if mkdir proto_files.lock 2>/dev/null; then \
## This code is being executed by the first process.
	  rm -f proto_files.ts; \
	  $(MAKE) $(AM_MAKEFLAGS) proto_files.ts; \
	  result=$$?; rm -rf proto_files.lock; exit $$result; \
	else \
## This code is being executed by the follower processes.
## Wait until the first process is done.
	  while test -d proto_files.lock; do sleep 1; done; \
## Succeed if and only if the first process succeeded.
	    test -f proto_files.ts; \
	  fi; \
	fi

AM_CPPFLAGS += -Icpp_out -Igrpc_out \
-I$(builddir)/cpp_out -I$(builddir)/grpc_out

nodist_libbm_grpc_dataplane_la_SOURCES = $(proto_cpp_files) $(proto_grpc_files)
# no warnings or Werror for auto-generated sources
libbm_grpc_dataplane_la_CXXFLAGS =

CLEANFILES = $(BUILT_SOURCES) proto_files.ts
