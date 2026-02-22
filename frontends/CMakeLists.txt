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
  p4/actionsInlining.cpp
  p4/callGraph.cpp
  p4/checkConstants.cpp
  p4/checkCoreMethods.cpp
  p4/checkNamedArgs.cpp
  p4/createBuiltins.cpp
  p4/def_use.cpp
  p4/defaultArguments.cpp
  p4/defaultValues.cpp
  p4/deprecated.cpp
  p4/directCalls.cpp
  p4/dontcareArgs.cpp
  p4/enumInstance.cpp
  p4/entryPriorities.cpp
  p4/evaluator/evaluator.cpp
  p4/evaluator/substituteParameters.cpp
  p4/externInstance.cpp
  p4/frontend.cpp
  p4/functionsInlining.cpp
  p4/hierarchicalNames.cpp
  p4/duplicateActionControlPlaneNameCheck.cpp
  p4/inlining.cpp
  p4/localizeActions.cpp
  p4/methodInstance.cpp
  p4/moveConstructors.cpp
  p4/moveDeclarations.cpp
  p4/parameterSubstitution.cpp
  p4/parseAnnotations.cpp
  p4/parserCallGraph.cpp
  p4/parserControlFlow.cpp
  p4/reassociation.cpp
  p4/redundantParsers.cpp
  p4/removeParameters.cpp
  p4/removeReturns.cpp
  p4/reservedWords.cpp
  p4/resetHeaders.cpp
  p4/setHeaders.cpp
  p4/sideEffects.cpp
  p4/simplify.cpp
  p4/simplifyDefUse.cpp
  p4/simplifyParsers.cpp
  p4/simplifySwitch.cpp
  p4/specialize.cpp
  p4/specializeGenericFunctions.cpp
  p4/specializeGenericTypes.cpp
  p4/strengthReduction.cpp
  p4/structInitializers.cpp
  p4/switchAddDefault.cpp
  p4/symbol_table.cpp
  p4/tableApply.cpp
  p4/tableKeyNames.cpp
  p4/ternaryBool.cpp
  p4/toP4/toP4.cpp
  p4/typeChecking/bindVariables.cpp
  p4/typeChecking/syntacticEquivalence.cpp
  p4/typeChecking/typeChecker.cpp
  p4/typeChecking/typeCheckDecl.cpp  
  p4/typeChecking/typeCheckExpr.cpp
  p4/typeChecking/typeCheckStmt.cpp
  p4/typeChecking/typeCheckTypes.cpp
  p4/typeChecking/typeConstraints.cpp
  p4/typeChecking/typeSubstitution.cpp
  p4/typeChecking/typeSubstitutionVisitor.cpp
  p4/typeChecking/typeUnification.cpp
  p4/typeChecking/typeInference.cpp
  p4/typeChecking/readOnlyTypeInference.cpp
  p4/typeMap.cpp
  p4/uniqueNames.cpp
  p4/unusedDeclarations.cpp
  p4/uselessCasts.cpp
  p4/validateParsedProgram.cpp
  p4/metrics/metricsPassManager.cpp
  p4/metrics/linesOfCodeMetric.cpp
  p4/metrics/cyclomaticComplexity.cpp
  p4/metrics/unusedCodeMetric.cpp
  p4/metrics/nestingDepthMetric.cpp
  p4/metrics/halsteadMetrics.cpp
  p4/metrics/headerMetrics.cpp
  p4/metrics/headerPacketMetrics.cpp
  p4/metrics/matchActionTableMetrics.cpp
  p4/metrics/parserMetrics.cpp
  p4/metrics/inlinedActionsMetric.cpp
  p4/metrics/externalObjectsMetric.cpp
  p4/metrics/exportMetrics.cpp
  )

set (COMMON_FRONTEND_SRCS
  common/applyOptionsPragmas.cpp
  common/constantFolding.cpp
  common/constantParsing.cpp
  common/options.cpp
  common/parser_options.cpp
  common/parseInput.cpp
  common/resolveReferences/referenceMap.cpp
  common/resolveReferences/resolveReferences.cpp
  )

set (PARSERS_SRCS
  parsers/parserDriver.cpp
  parsers/p4/p4AnnotationLexer.cpp
  )

macro(add_parser pname)
  add_custom_target (mk${pname}dirs
    ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/parsers/${pname})

  # CMake generates a file with the same name automatically and then issues a warning because the
  # file gets overwritten, so we need this code to avoid warning spew during compilation.
  BISON_TARGET (${pname}Parser parsers/${pname}/${pname}parser.ypp ${CMAKE_CURRENT_BINARY_DIR}/parsers/${pname}/${pname}parser.cpp VERBOSE COMPILE_FLAGS "-Werror=conflicts-sr -Werror=conflicts-rr")

  # regardless of the output name, flex puts out yy.lex.c, so we use a custom command to generate
  # the file to stdout and redirect to the desired name
  set (FLEX_${pname}Lexer_OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/parsers/${pname}/${pname}lexer.cc)
  set (FLEX_${pname}_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/parsers/${pname}/${pname}lexer.ll)
  add_custom_command (OUTPUT ${FLEX_${pname}Lexer_OUTPUTS}
    COMMAND ${FLEX_EXECUTABLE} -t ${FLEX_${pname}_INPUT} > ${FLEX_${pname}Lexer_OUTPUTS}
    DEPENDS ${BISON_${pname}Parser_OUTPUTS} ${FLEX_${pname}_INPUT}
    COMMENT "Running Flex on parsers/${pname}")

  set (${pname}PARSER_GEN_SRCS
    ${FLEX_${pname}Lexer_OUTPUTS}
    ${BISON_${pname}Parser_OUTPUTS}
    )
  set_source_files_properties(${${pname}PARSER_GEN_SRCS} PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE)

endmacro(add_parser)



add_parser(p4)
set(FRONTEND_GEN_SOURCES ${p4PARSER_GEN_SRCS})

if (SUPPORT_P4_14)
add_parser(v1)
set (FRONTEND_GEN_SOURCES ${FRONTEND_GEN_SOURCES} ${v1PARSER_GEN_SRCS})
endif ()


add_library (frontend-parser-gen OBJECT ${FRONTEND_GEN_SOURCES})
target_link_libraries (frontend-parser-gen
  # Make sure to also expose the compile flags associated with the ir library.
  PUBLIC ir
  # These libraries are exposed by a header.
  PUBLIC p4ctoolkit
)
add_dependencies (frontend-parser-gen mkp4dirs ir-generated)



set (FRONTEND_SOURCES
  ${COMMON_FRONTEND_SRCS}
  ${PARSERS_SRCS}
  ${P4_FRONTEND_SRCS}
  )

if (SUPPORT_P4_14)
  add_dependencies (frontend-parser-gen mkv1dirs)

  set (V1_FRONTEND_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/parsers/parserDriver_v1.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/p4/typeChecking/typeCheckExpr_v1.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/p4-14/typecheck.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/p4-14/header_type.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/p4-14/v1.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/p4-14/fromv1.0/converters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/p4-14/fromv1.0/programStructure.cpp
  )
  set (FRONTEND_SOURCES ${FRONTEND_SOURCES} ${V1_FRONTEND_SRCS})
  set (IR_DEF_FILES ${IR_DEF_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/p4-14/ir-v1.def PARENT_SCOPE)
endif ()


if (FLEX_INCLUDE_DIRS)
  include_directories (${FLEX_INCLUDE_DIRS})
endif ()
add_library (frontend STATIC ${FRONTEND_SOURCES})
target_link_libraries (frontend
  PRIVATE absl::strings
  # These libraries are exposed by a header.
  PUBLIC frontend-parser-gen
  PUBLIC absl::flat_hash_set
  PUBLIC absl::flat_hash_map
)
