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
  actionsInlining.cpp
  actionSynthesis.cpp
  copyStructures.cpp
  convertEnums.cpp
  eliminateTuples.cpp
  nestedStructs.cpp
  inlining.cpp
  interpreter.cpp
  local_copyprop.cpp
  localizeActions.cpp
  moveConstructors.cpp
  predication.cpp
  parserUnroll.cpp
  removeParameters.cpp
  removeReturns.cpp
  removeUnusedParameters.cpp
  removeLeftSlices.cpp
  simplifyKey.cpp
  simplifySelectCases.cpp
  simplifySelectList.cpp
  synthesizeValidField.cpp
  removeSelectBooleans.cpp
  validateProperties.cpp
  expandLookahead.cpp
  noMatch.cpp
  tableHit.cpp
  expandEmit.cpp
  dontcareArgs.cpp
  )

set (MIDEND_HDRS
  actionsInlining.h
  actionSynthesis.h
  compileTimeOps.h
  convertEnums.h
  copyStructures.h
  eliminateTuples.h
  expandEmit.h
  expandLookahead.h
  expr_uses.h
  has_side_effects.h
  inlining.h
  interpreter.h
  local_copyprop.h
  localizeActions.h
  midEndLast.h
  moveConstructors.h
  noMatch.h
  predication.h
  nestedStructs.h
  parserUnroll.h
  removeLeftSlices.h
  removeParameters.h
  removeReturns.h
  removeSelectBooleans.h
  removeUnusedParameters.h
  simplifyKey.h
  simplifySelectCases.h
  simplifySelectList.h
  synthesizeValidField.h
  tableHit.h
  validateProperties.h
  dontcareArgs.h
  )

add_cpplint_files (${CMAKE_CURRENT_SOURCE_DIR} "${MIDEND_SRCS};${MIDEND_HDRS}")

build_unified(MIDEND_SRCS)
add_library (midend STATIC ${MIDEND_SRCS})
add_dependencies(midend genIR)
