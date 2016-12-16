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

cpplint_FILES += $(libp4ctoolkit_a_UNIFIED) $(libp4ctoolkit_a_NONUNIFIED)
