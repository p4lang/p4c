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

set (MIDEND_SRCS
  actionSynthesis.cpp
  actionsInlining.cpp
  convertEnums.cpp
  copyStructures.cpp
  dontcareArgs.cpp
  eliminateTuples.cpp
  expandEmit.cpp
  expandLookahead.cpp
  hierarchicalNames.cpp
  inlining.cpp
  interpreter.cpp
  local_copyprop.cpp
  localizeActions.cpp
  moveConstructors.cpp
  nestedStructs.cpp
  noMatch.cpp
  parserUnroll.cpp
  predication.cpp
  removeLeftSlices.cpp
  removeParameters.cpp
  removeReturns.cpp
  removeSelectBooleans.cpp
  removeUnusedParameters.cpp
  simplifyKey.cpp
  simplifySelectCases.cpp
  simplifySelectList.cpp
  synthesizeValidField.cpp
  tableHit.cpp
  validateProperties.cpp
  )

set (MIDEND_HDRS
  actionSynthesis.h
  actionsInlining.h
  compileTimeOps.h
  convertEnums.h
  copyStructures.h
  dontcareArgs.h
  eliminateTuples.h
  expandEmit.h
  expandLookahead.h
  expr_uses.h
  has_side_effects.h
  hierarchicalNames.h
  inlining.h
  interpreter.h
  local_copyprop.h
  midEndLast.h
  moveConstructors.h
  nestedStructs.h
  noMatch.h
  parserUnroll.h
  predication.h
  removeExits.h
  removeLeftSlices.h
  removeParameters.h
  removeSelectBooleans.h
  removeUnusedParameters.h
  simplifyKey.h
  simplifySelectCases.h
  simplifySelectList.h
  synthesizeValidField.h
  tableHit.h
  validateProperties.h
  )

add_cpplint_files (${CMAKE_CURRENT_SOURCE_DIR} "${MIDEND_SRCS};${MIDEND_HDRS}")

build_unified(MIDEND_SRCS)
add_library (midend STATIC ${MIDEND_SRCS})
add_dependencies(midend genIR)
