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

ir_UNIFIED = \
	ir/base.cpp \
	ir/dbprint.cpp \
	ir/dbprint-expression.cpp \
	ir/dbprint-stmt.cpp \
	ir/dbprint-type.cpp \
	ir/dbprint-p4.cpp \
	ir/dump.cpp \
	ir/expression.cpp \
	ir/ir.cpp \
	ir/node.cpp \
	ir/pass_manager.cpp \
	ir/type.cpp \
	ir/v1.cpp \
	ir/visitor.cpp \
	ir/write_context.cpp

noinst_HEADERS += \
	ir/configuration.h \
	ir/dbprint.h \
	ir/dump.h \
	ir/id.h \
	ir/indexed_vector.h \
	ir/ir-inline.h \
	ir/ir-tree-macros.h \
	ir/ir.h \
	ir/json_generator.h \
	ir/json_loader.h \
	ir/json_parser.h \
	ir/namemap.h \
	ir/node.h \
	ir/nodemap.h \
	ir/pass_manager.h \
	ir/std.h \
	ir/vector.h \
	ir/visitor.h

ir_DEF_FILES += \
    $(srcdir)/ir/base.def \
    $(srcdir)/ir/type.def \
    $(srcdir)/ir/expression.def \
    $(srcdir)/ir/ir.def \
    $(srcdir)/ir/v1.def

cpplint_FILES += $(ir_UNIFIED) $(ir_NONUNIFIED)
