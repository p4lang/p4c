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

p4_frontend_UNIFIED = \
	frontends/p4/frontend.cpp \
	frontends/p4/createBuiltins.cpp \
	frontends/p4/def_use.cpp \
	frontends/p4/validateParsedProgram.cpp \
	frontends/p4/symbol_table.cpp \
	frontends/p4/toP4/toP4.cpp \
	frontends/p4/callGraph.cpp \
	frontends/p4/typeChecking/typeChecker.cpp \
	frontends/p4/typeChecking/bindVariables.cpp \
	frontends/p4/typeChecking/typeConstraints.cpp \
	frontends/p4/typeChecking/typeUnification.cpp \
	frontends/p4/typeChecking/syntacticEquivalence.cpp \
	frontends/p4/strengthReduction.cpp \
	frontends/p4/unusedDeclarations.cpp \
	frontends/p4/evaluator/evaluator.cpp \
	frontends/p4/evaluator/substituteParameters.cpp \
	frontends/p4/parserControlFlow.cpp \
	frontends/p4/reservedWords.cpp \
	frontends/p4/coreLibrary.cpp \
	frontends/p4/methodInstance.cpp \
	frontends/p4/parserCallGraph.cpp \
	frontends/p4/tableApply.cpp \
	frontends/p4/simplify.cpp \
	frontends/p4/substitution.cpp \
	frontends/p4/substitutionVisitor.cpp \
	frontends/p4/fromv1.0/programStructure.cpp \
	frontends/p4/fromv1.0/v1model.cpp \
	frontends/p4/fromv1.0/converters.cpp \
	frontends/p4/typeMap.cpp \
	frontends/p4/simplifyDefUse.cpp \
	frontends/p4/sideEffects.cpp \
	frontends/p4/simplifyParsers.cpp \
	frontends/p4/uniqueNames.cpp \
	frontends/p4/resetHeaders.cpp \
	frontends/p4/moveDeclarations.cpp \
	frontends/p4/specialize.cpp

p4_frontend_NONUNIFIED = \
	frontends/p4/p4-parse.ypp

BUILT_SOURCES += \
        frontends/p4/p4-lex.c \
	frontends/p4/p4-parse.cpp

noinst_HEADERS += \
	frontends/p4/callGraph.h \
	frontends/p4/cloner.h \
	frontends/p4/coreLibrary.h \
	frontends/p4/createBuiltins.h \
	frontends/p4/def_use.h \
	frontends/p4/enumInstance.h \
	frontends/p4/evaluator/evaluator.h \
	frontends/p4/evaluator/substituteParameters.h \
	frontends/p4/fromv1.0/converters.h \
	frontends/p4/fromv1.0/programStructure.h \
	frontends/p4/fromv1.0/v1model.h \
	frontends/p4/frontend.h \
	frontends/p4/methodInstance.h \
	frontends/p4/moveDeclarations.h \
	frontends/p4/p4-parse.h \
	frontends/p4/parameterSubstitution.h \
	frontends/p4/parserCallGraph.h \
	frontends/p4/parserControlFlow.h \
	frontends/p4/reservedWords.h \
	frontends/p4/resetHeaders.h \
	frontends/p4/sideEffects.h \
	frontends/p4/simplify.h \
	frontends/p4/simplifyDefUse.h \
	frontends/p4/simplifyParsers.h \
	frontends/p4/specialize.h \
	frontends/p4/strengthReduction.h \
	frontends/p4/substitution.h \
	frontends/p4/substitutionVisitor.h \
	frontends/p4/symbol_table.h \
	frontends/p4/tableApply.h \
	frontends/p4/toP4/toP4.h \
	frontends/p4/typeChecking/bindVariables.h \
	frontends/p4/typeChecking/syntacticEquivalence.h \
	frontends/p4/typeChecking/typeChecker.h \
	frontends/p4/typeChecking/typeConstraints.h \
	frontends/p4/typeChecking/typeUnification.h \
	frontends/p4/typeMap.h \
	frontends/p4/uniqueNames.h \
	frontends/p4/unusedDeclarations.h \
	frontends/p4/validateParsedProgram.h

cpplint_FILES += frontends/p4/p4-lex.l \
		 $(p4_frontend_UNIFIED) \
		 $(p4_frontend_NONUNIFIED)


common_frontend_UNIFIED = \
	frontends/common/options.cpp \
	frontends/common/constantFolding.cpp \
	frontends/common/resolveReferences/referenceMap.cpp \
	frontends/common/resolveReferences/resolveReferences.cpp \
	frontends/common/parseInput.cpp \
	frontends/common/constantParsing.cpp

noinst_HEADERS += \
	frontends/common/constantFolding.h \
	frontends/common/constantParsing.h \
	frontends/common/model.h \
	frontends/common/name_gateways.h \
	frontends/common/options.h \
	frontends/common/parseInput.h \
	frontends/common/programMap.h \
	frontends/common/resolveReferences/referenceMap.h \
	frontends/common/resolveReferences/resolveReferences.h

cpplint_FILES += $(common_frontend_UNIFIED) $(common_frontend_NONUNIFIED)


v1_frontend_UNIFIED = \
	frontends/p4-14/intrinsic.cpp \
	frontends/p4-14/typecheck.cpp \
	frontends/p4-14/header_type.cpp

v1_frontend_NONUNIFIED = \
	frontends/p4-14/p4-14-parse.ypp

ir_DEF_FILES += $(srcdir)/frontends/p4-14/ir-v1.def

BUILT_SOURCES += \
        frontends/p4-14/p4-14-lex.c \
	frontends/p4-14/p4-14-parse.cpp

noinst_HEADERS += \
	frontends/p4-14/header_type.h \
	frontends/p4-14/inline_control_flow.h \
	frontends/p4-14/p4-14-parse.h \
	frontends/p4-14/typecheck.h

cpplint_FILES += frontends/p4-14/p4-14-lex.l \
		 $(v1_frontend_UNIFIED) \
		 $(v1_frontend_NONUNIFIED)

frontend_SOURCES = \
	$(p4_frontend_SOURCES) \
	$(common_frontend_SOURCES) \
	$(v1_frontend_SOURCES)
