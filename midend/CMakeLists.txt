# SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
# Copyright 2013-present Barefoot Networks, Inc.
#
# SPDX-License-Identifier: Apache-2.0

set (MIDEND_SRCS
  actionSynthesis.cpp
  booleanKeys.cpp
  complexComparison.cpp
  convertEnums.cpp
  convertErrors.cpp
  copyStructures.cpp
  coverage.cpp
  checkTableEntries.cpp
  def_use.cpp
  eliminateActionRun.cpp
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
  simplifyExternMethod.cpp
  simplifyKey.cpp
  simplifySelectCases.cpp
  simplifySelectList.cpp
  singleArgumentSelect.cpp
  tableHit.cpp
  unrollLoops.cpp
  validateProperties.cpp
  )

set (MIDEND_HDRS
  actionSynthesis.h
  booleanKeys.h
  checkSize.h
  checkExternInvocationCommon.h
  checkTableEntries.h
  compileTimeOps.h
  complexComparison.h
  convertEnums.h
  convertErrors.h
  copyStructures.h
  coverage.h
  def_use.h
  eliminateActionRun.h
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
  simplifyExternMethod.h
  simplifyKey.h
  simplifySelectCases.h
  simplifySelectList.h
  singleArgumentSelect.h
  tableHit.h
  unrollLoops.h
  validateProperties.h
  )


add_library (midend STATIC ${MIDEND_SRCS})
target_link_libraries(midend
  # For TypeMap / RefMap
  PRIVATE frontend
)
add_dependencies(midend genIR ir-generated)
