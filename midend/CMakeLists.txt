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
  complexComparison.cpp
  convertEnums.cpp
  copyStructures.cpp
  eliminateTuples.cpp
  eliminateNewtype.cpp
  eliminateSerEnums.cpp
  expandEmit.cpp
  expandLookahead.cpp
  flattenHeaders.cpp
  flattenInterfaceStructs.cpp
  interpreter.cpp
  local_copyprop.cpp
  nestedStructs.cpp
  noMatch.cpp
  orderArguments.cpp
  parserUnroll.cpp
  predication.cpp
  removeExits.cpp
  removeLeftSlices.cpp
  removeParameters.cpp
  removeSelectBooleans.cpp
  removeUnusedParameters.cpp
  simplifyBitwise.cpp
  simplifyKey.cpp
  simplifySelectCases.cpp
  simplifySelectList.cpp
  singleArgumentSelect.cpp
  tableHit.cpp
  validateProperties.cpp
  )

set (MIDEND_HDRS
  actionSynthesis.h
  checkSize.h
  compileTimeOps.h
  complexComparison.h
  convertEnums.h
  copyStructures.h
  eliminateTuples.h
  eliminateNewtype.h
  eliminateSerEnums.h
  expandEmit.h
  expandLookahead.h
  expr_uses.h
  flattenInterfaceStructs.h
  flattenHeaders.h  
  has_side_effects.h
  interpreter.h
  local_copyprop.h
  midEndLast.h
  nestedStructs.h
  noMatch.h
  orderArguments.h
  parserUnroll.h
  predication.h
  removeExits.h
  removeLeftSlices.h
  removeParameters.h
  removeSelectBooleans.h
  removeUnusedParameters.h
  simplifyBitwise.h
  simplifyKey.h
  simplifySelectCases.h
  simplifySelectList.h
  singleArgumentSelect.h
  tableHit.h
  validateProperties.h
  )

add_cpplint_files (${CMAKE_CURRENT_SOURCE_DIR} "${MIDEND_SRCS};${MIDEND_HDRS}")

build_unified(MIDEND_SRCS)
add_library (midend STATIC ${MIDEND_SRCS})
add_dependencies(midend genIR)
