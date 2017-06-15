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

if ENABLE_P4RUNTIME_TO_PD

bin_SCRIPTS += p4runtime-to-pd

# p4runtime-to-pd is a script to generate PD files directly from P4Runtime. The
# output is compatible with p4c-bm's output.
p4runtime_to_pddir=$(pkgdatadir)/p4runtime_to_pd_src
p4runtime_to_pd_PYTHON = \
		p4runtime-to-pd/__init__.py \
		p4runtime-to-pd/main.py \
		p4runtime-to-pd/tenjin_wrapper.py \
		control-plane/p4/config/p4info_pb2.py \
		control-plane/p4/config/v1model_pb2.py

# These are the tenjin templates that p4runtime_to_pd uses. The directory
# structure is important here, so we install them with 'nobase_'.
nobase_p4runtime_to_pd_PYTHON = \
		p4runtime-to-pd/templates/pd/pd.h \
		p4runtime-to-pd/templates/pd/pd_counters.h \
		p4runtime-to-pd/templates/pd/pd_learning.h \
		p4runtime-to-pd/templates/pd/pd_meters.h \
		p4runtime-to-pd/templates/pd/pd_registers.h \
		p4runtime-to-pd/templates/pd/pd_tables.h \
		p4runtime-to-pd/templates/pd/pd_types.h \
		p4runtime-to-pd/templates/src/pd.cpp \
		p4runtime-to-pd/templates/src/pd_ageing.cpp \
		p4runtime-to-pd/templates/src/pd_client.h \
		p4runtime-to-pd/templates/src/pd_counters.cpp \
		p4runtime-to-pd/templates/src/pd_learning.cpp \
		p4runtime-to-pd/templates/src/pd_meters.cpp \
		p4runtime-to-pd/templates/src/pd_registers.cpp \
		p4runtime-to-pd/templates/src/pd_tables.cpp \
		p4runtime-to-pd/templates/thrift-src/p4_pd_rpc_server.ipp \
		p4runtime-to-pd/templates/thrift-src/pd_rpc_server.cpp \
		p4runtime-to-pd/templates/thrift-src/pd_rpc_server.h \
		p4runtime-to-pd/templates/thrift/p4_pd_rpc.thrift

endif
