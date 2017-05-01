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
	frontends/p4/modelInstances.cpp \
	frontends/p4/externInstance.cpp \
	frontends/p4/methodInstance.cpp \
	frontends/p4/parserCallGraph.cpp \
	frontends/p4/tableApply.cpp \
	frontends/p4/simplify.cpp \
	frontends/p4/substitution.cpp \
	frontends/p4/substitutionVisitor.cpp \
	frontends/p4/fromv1.0/programStructure.cpp \
	frontends/p4/fromv1.0/converters.cpp \
	frontends/p4/typeMap.cpp \
	frontends/p4/simplifyDefUse.cpp \
	frontends/p4/sideEffects.cpp \
	frontends/p4/simplifyParsers.cpp \
	frontends/p4/uniqueNames.cpp \
	frontends/p4/resetHeaders.cpp \
	frontends/p4/moveDeclarations.cpp \
	frontends/p4/tableKeyNames.cpp \
	frontends/p4/specialize.cpp \
	frontends/p4/uselessCasts.cpp \
	frontends/p4/directCalls.cpp \
	frontends/p4/setHeaders.cpp

noinst_HEADERS += \
	frontends/p4/callGraph.h \
	frontends/p4/cloner.h \
	frontends/p4/coreLibrary.h \
	frontends/p4/createBuiltins.h \
	frontends/p4/def_use.h \
	frontends/p4/enumInstance.h \
	frontends/p4/evaluator/evaluator.h \
	frontends/p4/evaluator/substituteParameters.h \
	frontends/p4/externInstance.h \
	frontends/p4/fromv1.0/converters.h \
	frontends/p4/fromv1.0/programStructure.h \
	frontends/p4/fromv1.0/v1model.h \
	frontends/p4/frontend.h \
	frontends/p4/methodInstance.h \
	frontends/p4/moveDeclarations.h \
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
	frontends/p4/tableKeyNames.h \
	frontends/p4/validateParsedProgram.h \
	frontends/p4/uselessCasts.h \
	frontends/p4/directCalls.h \
	frontends/p4/setHeaders.h

cpplint_FILES += $(p4_frontend_UNIFIED) \
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
	frontends/p4-14/typecheck.cpp \
	frontends/p4-14/header_type.cpp

ir_DEF_FILES += $(srcdir)/frontends/p4-14/ir-v1.def

noinst_HEADERS += \
	frontends/p4-14/header_type.h \
	frontends/p4-14/inline_control_flow.h \
	frontends/p4-14/typecheck.h

cpplint_FILES += $(v1_frontend_UNIFIED) \
		 $(v1_frontend_NONUNIFIED)

parsers_UNIFIED = \
	frontends/parsers/parserDriver.cpp

parsers_NONUNIFIED = \
        frontends/parsers/p4/p4lexer.ll \
	frontends/parsers/p4/p4parser.ypp \
        frontends/parsers/v1/v1lexer.ll \
	frontends/parsers/v1/v1parser.ypp

BUILT_SOURCES += \
	frontends/parsers/p4/p4parser.hpp \
	frontends/parsers/v1/v1parser.hpp

noinst_HEADERS += \
	frontends/parsers/p4/p4lexer.hpp \
	frontends/parsers/v1/v1lexer.hpp

noinst_HEADERS += \
	frontends/parsers/parserDriver.h

cpplint_FILES += $(parsers_UNIFIED) \
		 $(parsers_NONUNIFIED)

frontend_SOURCES = \
	$(common_frontend_SOURCES) \
	$(parsers_SOURCES) \
	$(p4_frontend_SOURCES) \
	$(v1_frontend_SOURCES)
