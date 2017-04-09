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

# Makefile for mid-ends

midend_UNIFIED = \
	midend/actionsInlining.cpp \
	midend/actionSynthesis.cpp \
	midend/copyStructures.cpp \
	midend/convertEnums.cpp \
	midend/eliminateTuples.cpp \
	midend/nestedStructs.cpp \
	midend/inlining.cpp \
	midend/interpreter.cpp \
	midend/local_copyprop.cpp \
	midend/localizeActions.cpp \
	midend/moveConstructors.cpp \
	midend/predication.cpp \
	midend/parserUnroll.cpp \
	midend/removeParameters.cpp \
	midend/removeReturns.cpp \
	midend/removeLeftSlices.cpp \
	midend/simplifyKey.cpp \
	midend/simplifySelectCases.cpp \
	midend/simplifySelectList.cpp \
        midend/removeSelectBooleans.cpp \
	midend/validateProperties.cpp \
	midend/expandLookahead.cpp \
	midend/noMatch.cpp \
	midend/tableHit.cpp

noinst_HEADERS += \
	midend/actionsInlining.h \
	midend/actionSynthesis.h \
	midend/compileTimeOps.h \
	midend/convertEnums.h \
	midend/copyStructures.h \
	midend/eliminateTuples.h \
	midend/expr_uses.h \
	midend/has_side_effects.h \
	midend/inlining.h \
	midend/interpreter.h \
	midend/local_copyprop.h \
	midend/localizeActions.h \
	midend/moveConstructors.h \
	midend/predication.h \
	midend/nestedStructs.h \
	midend/parserUnroll.h \
	midend/removeLeftSlices.h \
	midend/removeParameters.h \
	midend/removeReturns.h \
	midend/simplifyKey.h \
	midend/simplifySelectCases.h \
	midend/simplifySelectList.h \
        midend/removeSelectBooleans.h \
	midend/validateProperties.h \
	midend/expandLookahead.h \
	midend/noMatch.h \
	midend/tableHit.h \
	midend/midEndLast.h

cpplint_FILES += $(midend_UNIFIED) $(midend_NONUNIFIED)
