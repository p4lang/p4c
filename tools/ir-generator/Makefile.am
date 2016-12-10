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

irgenerator_UNIFIED = \
	tools/ir-generator/generator.cpp \
	tools/ir-generator/irclass.cpp \
	tools/ir-generator/methods.cpp \
	tools/ir-generator/type.cpp

irgenerator_NONUNIFIED = \
	tools/ir-generator/ir-generator.ypp

BUILT_SOURCES += \
	tools/ir-generator/ir-generator-lex.c

irgenerator_LDADD = libp4ctoolkit.a

noinst_PROGRAMS += irgenerator

cpplint_FILES += $(irgenerator_UNIFIED) $(irgenerator_NONUNIFIED)
