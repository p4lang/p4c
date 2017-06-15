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

# The standard P4Runtime protocol buffers message definitions live in the PI
# repo, which is included in this repo as a submodule.
P4RUNTIME_STD_DIR = $(abs_srcdir)/%reldir%/PI/proto
control_plane_proto_DIRS += $(P4RUNTIME_STD_DIR)
control_plane_proto_FILES += $(P4RUNTIME_STD_DIR)/p4/config/p4info.proto
control_plane_SOURCES += control-plane/p4/config/p4info.pb.cc

# Architecture-specific P4Runtime extern definitions.
P4RUNTIME_ARCH_DIR = $(abs_srcdir)/%reldir%/proto
control_plane_proto_DIRS += $(P4RUNTIME_ARCH_DIR)
control_plane_proto_FILES += $(P4RUNTIME_ARCH_DIR)/p4/config/v1model.proto
control_plane_SOURCES += control-plane/p4/config/v1model.pb.cc

# Add support for automake's verbosity options for protoc.
V_PROTOC = $(__v_PROTOC_$(V))
__v_PROTOC_ = $(__v_PROTOC_$(AM_DEFAULT_VERBOSITY))
__v_PROTOC_0 = @echo "  PROTOC   " $?;
__v_PROTOC_1 =

# Generate source code from the .proto definitions using protoc. The output is
# placed in the build directory inside `control-plane`. For example,
# p4info.proto produces:
#   control-plane/p4/config/p4info.pb.h
#   control-plane/p4/config/p4info.pb.cc
# These files aren't automatically included in libcontrolplane; the `.cc` files
# need to be added to control_plane_SOURCES.
PROTOFLAGS = $(addprefix -I,$(control_plane_proto_DIRS))
.protoc.stamp: $(control_plane_proto_FILES)
	@mkdir -p control-plane
	$(V_PROTOC) $(PROTOC) $(PROTOFLAGS) $^ \
		--cpp_out control-plane \
		--python_out control-plane
	@touch $@

BUILT_SOURCES += \
	.protoc.stamp

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
	control-plane/p4RuntimeSerializer.cpp \
	$(control_plane_SOURCES)

noinst_HEADERS += \
	control-plane/p4RuntimeSerializer.h

# Work around a bug in protoc v.3.0 which generates unused 'deterministic'
# parameters that cause a lot of warning spew during the build process. This is
# fixed in v.3.1, but unfortunately there are p4c extensions which use Google's
# or-tools library, and there is no release of or-tools which is compatible with
# protoc v.3.1 at this time.
libcontrolplane_la_CXXFLAGS = $(AM_CXXFLAGS) -Wno-unused-parameter
