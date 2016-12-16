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

noinst_LIBRARIES += libp4ctoolkit.a
libp4ctoolkit_a_UNIFIED = \
	lib/bitvec.cpp \
	lib/crash.cpp \
	lib/cstring.cpp \
	lib/error.cpp \
	lib/gc.cpp \
	lib/gmputil.cpp \
	lib/hex.cpp\
	lib/indent.cpp \
	lib/json.cpp \
	lib/log.cpp \
	lib/match.cpp \
	lib/nullstream.cpp \
	lib/options.cpp \
	lib/path.cpp \
	lib/source_file.cpp \
	lib/stringify.cpp

noinst_HEADERS += \
	lib/algorithm.h \
	lib/alloc.h \
	lib/bitops.h \
	lib/bitrange.h \
	lib/bitvec.h \
	lib/crash.h \
	lib/cstring.h \
	lib/default.h \
	lib/enumerator.h \
	lib/error.h \
	lib/exceptions.h \
	lib/gc.h \
	lib/gmputil.h \
	lib/hex.h \
	lib/indent.h \
	lib/json.h \
	lib/log.h \
	lib/ltbitmatrix.h \
	lib/map.h \
	lib/match.h \
	lib/n4.h \
	lib/null.h \
	lib/nullstream.h \
	lib/options.h \
	lib/ordered_map.h \
	lib/ordered_set.h \
	lib/path.h \
	lib/range.h \
	lib/set.h \
	lib/source_file.h \
	lib/sourceCodeBuilder.h \
	lib/stringify.h \
	lib/stringref.h \
	lib/symbitmatrix.h

cpplint_FILES += $(libp4ctoolkit_a_UNIFIED) $(libp4ctoolkit_a_NONUNIFIED)
