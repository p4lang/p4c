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

ir_SOURCES += \
	ir/base.cpp \
	ir/configuration.h \
	ir/dbprint.cpp \
	ir/dbprint-expression.cpp \
	ir/dbprint.h \
	ir/dbprint-p4.cpp \
	ir/dump.cpp \
	ir/expression.cpp \
	ir/id.h \
	ir/ir.cpp \
	ir/ir-generated.cpp \
	ir/json_parser.h \
	ir/ir.h \
	ir/ir-inline.h \
	ir/namemap.h \
	ir/node.cpp \
	ir/node.h \
	ir/pass_manager.cpp \
	ir/pass_manager.h \
	ir/type.cpp \
	ir/v1.cpp \
	ir/vector.h \
	ir/indexed_vector.h \
	ir/visitor.cpp \
	ir/visitor.h \
	ir/write_context.cpp 

ir_DEF_FILES += \
    $(srcdir)/ir/base.def \
    $(srcdir)/ir/type.def \
    $(srcdir)/ir/expression.def \
    $(srcdir)/ir/ir.def \
    $(srcdir)/ir/v1.def

cpplint_FILES += $(ir_SOURCES)
