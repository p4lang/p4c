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

set (P4_FRONTEND_SRCS
  p4/frontend.cpp
  p4/createBuiltins.cpp
  p4/def_use.cpp
  p4/validateParsedProgram.cpp
  p4/symbol_table.cpp
  p4/toP4/toP4.cpp
  p4/callGraph.cpp
  p4/typeChecking/typeChecker.cpp
  p4/typeChecking/bindVariables.cpp
  p4/typeChecking/typeConstraints.cpp
  p4/typeChecking/typeUnification.cpp
  p4/typeChecking/syntacticEquivalence.cpp
  p4/strengthReduction.cpp
  p4/unusedDeclarations.cpp
  p4/evaluator/evaluator.cpp
  p4/evaluator/substituteParameters.cpp
  p4/parserControlFlow.cpp
  p4/reservedWords.cpp
  p4/modelInstances.cpp
  p4/externInstance.cpp
  p4/methodInstance.cpp
  p4/parserCallGraph.cpp
  p4/tableApply.cpp
  p4/simplify.cpp
  p4/substitution.cpp
  p4/substitutionVisitor.cpp
  p4/fromv1.0/programStructure.cpp
  p4/fromv1.0/converters.cpp
  p4/typeMap.cpp
  p4/simplifyDefUse.cpp
  p4/sideEffects.cpp
  p4/simplifyParsers.cpp
  p4/uniqueNames.cpp
  p4/resetHeaders.cpp
  p4/moveDeclarations.cpp
  p4/tableKeyNames.cpp
  p4/specialize.cpp
  p4/uselessCasts.cpp
  p4/directCalls.cpp
  p4/setHeaders.cpp
  )

set (P4_FRONTEND_HDRS
  p4/callGraph.h
  p4/cloner.h
  p4/coreLibrary.h
  p4/createBuiltins.h
  p4/def_use.h
  p4/enumInstance.h
  p4/evaluator/evaluator.h
  p4/evaluator/substituteParameters.h
  p4/externInstance.h
  p4/fromv1.0/converters.h
  p4/fromv1.0/programStructure.h
  p4/fromv1.0/v1model.h
  p4/frontend.h
  p4/methodInstance.h
  p4/moveDeclarations.h
  p4/parameterSubstitution.h
  p4/parserCallGraph.h
  p4/parserControlFlow.h
  p4/reservedWords.h
  p4/resetHeaders.h
  p4/sideEffects.h
  p4/simplify.h
  p4/simplifyDefUse.h
  p4/simplifyParsers.h
  p4/specialize.h
  p4/strengthReduction.h
  p4/substitution.h
  p4/substitutionVisitor.h
  p4/symbol_table.h
  p4/tableApply.h
  p4/toP4/toP4.h
  p4/typeChecking/bindVariables.h
  p4/typeChecking/syntacticEquivalence.h
  p4/typeChecking/typeChecker.h
  p4/typeChecking/typeConstraints.h
  p4/typeChecking/typeUnification.h
  p4/typeMap.h
  p4/uniqueNames.h
  p4/unusedDeclarations.h
  p4/tableKeyNames.h
  p4/validateParsedProgram.h
  p4/uselessCasts.h
  p4/directCalls.h
  p4/setHeaders.h
  )


set (COMMON_FRONTEND_SRCS
  common/options.cpp
  common/constantFolding.cpp
  common/resolveReferences/referenceMap.cpp
  common/resolveReferences/resolveReferences.cpp
  common/parseInput.cpp
  common/constantParsing.cpp
  )

set (COMMON_FRONTEND_HDRS
  common/constantFolding.h
  common/constantParsing.h
  common/model.h
  common/name_gateways.h
  common/options.h
  common/parseInput.h
  common/programMap.h
  common/resolveReferences/referenceMap.h
  common/resolveReferences/resolveReferences.h
  )


set (V1_FRONTEND_SRCS
  p4-14/typecheck.cpp
  p4-14/header_type.cpp
  )

set (IR_DEF_FILES ${IR_DEF_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/p4-14/ir-v1.def PARENT_SCOPE)

set (V1_FRONTEND_HDRS
  p4-14/header_type.h
  p4-14/inline_control_flow.h
  p4-14/typecheck.h
  )

set (PARSERS_SRCS
  parsers/parserDriver.cpp
  )

set (PARSER_HDRS
  parsers/parserDriver.h
  )

add_custom_target (mkV1dirs
  ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/parsers/v1)
BISON_TARGET (V1Parser parsers/v1/v1parser.ypp ${CMAKE_CURRENT_BINARY_DIR}/parsers/v1/v1parser.cpp)
# FLEX_TARGET (V1Lexer parsers/v1/v1lexer.ll ${CMAKE_CURRENT_BINARY_DIR}/parsers/v1/v1lexer.cc)
set (FLEX_V1Lexer_OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/parsers/v1/v1lexer.cc)
add_custom_command (OUTPUT ${FLEX_V1Lexer_OUTPUTS}
  COMMAND ${FLEX_EXECUTABLE} -t ${CMAKE_CURRENT_SOURCE_DIR}/parsers/v1/v1lexer.ll > ${FLEX_V1Lexer_OUTPUTS}
  COMMENT "Running Flex on parsers/v1")
# ADD_FLEX_BISON_DEPENDENCY(V1Lexer V1Parser)

set (V1PARSER_GEN_SRCS
    ${FLEX_V1Lexer_OUTPUTS}
    ${BISON_V1Parser_OUTPUTS}
  )

add_custom_target (mkP4dirs
  ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/parsers/p4)
BISON_TARGET (P4Parser parsers/p4/p4parser.ypp ${CMAKE_CURRENT_BINARY_DIR}/parsers/p4/p4parser.cpp)
# FLEX_TARGET (P4Lexer parsers/p4/p4lexer.ll ${CMAKE_CURRENT_BINARY_DIR}/parsers/p4/p4lexer.cc)
set (FLEX_P4Lexer_OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/parsers/p4/p4lexer.cc)
add_custom_command (OUTPUT ${FLEX_P4Lexer_OUTPUTS}
  COMMAND ${FLEX_EXECUTABLE} -t ${CMAKE_CURRENT_SOURCE_DIR}/parsers/p4/p4lexer.ll > ${FLEX_P4Lexer_OUTPUTS}
  COMMENT "Running Flex on parsers/p4")
# ADD_FLEX_BISON_DEPENDENCY(P4Lexer P4Parser)

set (P4PARSER_GEN_SRCS
    ${FLEX_P4Lexer_OUTPUTS}
    ${BISON_P4Parser_OUTPUTS}
  )

# BUILT_SOURCES += \
# 	parsers/p4/p4parser.hpp \
# 	parsers/v1/v1parser.hpp

# noinst_HEADERS += \
# 	parsers/p4/p4lexer.hpp \
# 	parsers/v1/v1lexer.hpp

set (FRONTEND_SOURCES
  ${COMMON_FRONTEND_SRCS}
  ${PARSERS_SRCS}
  ${P4_FRONTEND_SRCS}
  ${V1_FRONTEND_SRCS}
  ${P4PARSER_GEN_SRCS}
  ${V1PARSER_GEN_SRCS}
  ${IR_GENERATED_SRCS}
  )

set_source_files_properties(${IR_GENERATED_SRCS} PROPERTIES GENERATED TRUE)
set_source_files_properties(${V1PARSER_GEN_SRCS} PROPERTIES GENERATED TRUE)
set_source_files_properties(${P4PARSER_GEN_SRCS} PROPERTIES GENERATED TRUE)
add_library (frontend STATIC ${FRONTEND_SOURCES})
add_dependencies (frontend genIR)
add_dependencies (frontend mkP4dirs mkV1dirs)

set (FRONTEND_CPPLINT_FILES
  ${P4_FRONTEND_SRCS} ${P4_FRONTEND_HDRS}
  ${COMMON_FRONTEND_SRCS} ${COMMON_FRONTEND_HDRS}
  ${V1_FRONTEND_SRCS} ${V1_FRONTEND_HDRS}
  ${PARSERS_SRCS} ${PARSERS_HDRS}
  PARENT_SCOPE)
