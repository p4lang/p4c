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
  booleanKeys.cpp
  complexComparison.cpp
  convertEnums.cpp
  convertErrors.cpp
  copyStructures.cpp
  coverage.cpp
  eliminateInvalidHeaders.cpp
  eliminateNewtype.cpp
  eliminateSerEnums.cpp
  eliminateSwitch.cpp
  eliminateTuples.cpp
  eliminateTypedefs.cpp
  expandEmit.cpp
  expandLookahead.cpp
  fillEnumMap.cpp
  flattenHeaders.cpp
  flattenInterfaceStructs.cpp
  flattenLogMsg.cpp
  flattenUnions.cpp
  hsIndexSimplify.cpp
  interpreter.cpp
  global_copyprop.cpp
  local_copyprop.cpp
  nestedStructs.cpp
  noMatch.cpp
  orderArguments.cpp
  parserUnroll.cpp
  predication.cpp
  removeAssertAssume.cpp
  removeComplexExpressions.cpp
  removeExits.cpp
  removeLeftSlices.cpp
  removeMiss.cpp
  removeSelectBooleans.cpp
  replaceSelectRange.cpp
  removeUnusedParameters.cpp
  saturationElim.cpp
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
  booleanKeys.h
  checkSize.h
  checkExternInvocationCommon.h
  compileTimeOps.h
  complexComparison.h
  convertEnums.h
  convertErrors.h
  copyStructures.h
  coverage.h
  eliminateInvalidHeaders.h
  eliminateNewtype.h
  eliminateSerEnums.h
  eliminateSwitch.h
  eliminateTuples.h
  eliminateTypedefs.h
  expandEmit.h
  expandLookahead.h
  expr_uses.h
  fillEnumMap.h
  flattenHeaders.h
  flattenInterfaceStructs.h
  flattenUnions.h
  has_side_effects.h
  interpreter.h
  global_copyprop.h
  local_copyprop.h
  midEndLast.h
  nestedStructs.h
  noMatch.h
  orderArguments.h
  parserUnroll.h
  predication.h
  removeAssertAssume.h
  removeComplexExpressions.h
  removeExits.h
  removeLeftSlices.h
  removeMiss.h
  removeSelectBooleans.h
  removeUnusedParameters.h
  replaceSelectRange.h
  saturationElim.h
  simplifyBitwise.h
  simplifyKey.h
  simplifySelectCases.h
  simplifySelectList.h
  singleArgumentSelect.h
  tableHit.h
  validateProperties.h
  )


add_library (midend STATIC ${MIDEND_SRCS})
add_dependencies(midend genIR ir-generated)
