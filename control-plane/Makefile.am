# Copyright 2013-present Barefoot Networks, Inc. 
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# The `.proto` definitions live in the PI repo, which is included in this repo
# as a submodule.
PROTO_ROOT = $(abs_srcdir)/%reldir%/PI/proto
PROTOFLAGS = -I$(PROTO_ROOT)

# We need to generate `p4.config.P4Info` protobuf data, so pull in `p4info.proto`.
p4info_proto = $(PROTO_ROOT)/p4/config/p4info.proto

# Generate C++ source code from the `.proto` definitions using protoc.
# protoc generates multiple outputs at once, which is handled very awkwardly
# by make. We take the simplest approach here. For the alternatives see:
#   http://www.gnu.org/software/automake/manual/html_node/Multiple-Outputs.html
control-plane/p4/config/p4info.pb.cc: $(p4info_proto)
	@mkdir -p control-plane
	$(PROTOC) $^ --cpp_out control-plane $(PROTOFLAGS)
control-plane/p4/config/p4info.pb.h: $(p4info_proto) control-plane/p4/config/p4info.pb.cc

BUILT_SOURCES += \
	control-plane/p4/config/p4info.pb.cc \
	control-plane/p4/config/p4info.pb.h

# Create a separate library for the control plane API code. This allows libtool
# to automatically link $(PROTOBUF_LIBS) into every library and executable that
# transitively includes this code. If we didn't use a libtool library, we'd have
# to explicitly link in $(PROTOBUF_LIBS) when compiling every p4c frontend.
noinst_LTLIBRARIES += libcontrolplane.la
libcontrolplane_la_LIBADD = $(PROTOBUF_LIBS)
libcontrolplane_la_CPPFLAGS = \
	-I$(srcdir)/control-plane/PI/include \
	-Icontrol-plane \
	$(AM_CPPFLAGS)
libcontrolplane_la_SOURCES = \
	control-plane/p4/config/p4info.pb.cc \
	control-plane/p4RuntimeSerializer.cpp

noinst_HEADERS += \
	control-plane/p4RuntimeSerializer.h

# Work around a bug in protoc v.3.0 which generates unused 'deterministic'
# parameters that cause a lot of warning spew during the build process. This is
# fixed in v.3.1, but unfortunately there are p4c extensions which use Google's
# or-tools library, and there is no release of or-tools which is compatible with
# protoc v.3.1 at this time.
libcontrolplane_la_CXXFLAGS = $(AM_CXXFLAGS) -Wno-unused-parameter
