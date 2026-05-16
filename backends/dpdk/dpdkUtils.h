/*
 * Copyright 2022 Intel Corporation
 * SPDX-FileCopyrightText: 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_DPDK_DPDKUTILS_H_
#define BACKENDS_DPDK_DPDKUTILS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"

namespace P4::DPDK {
bool isSimpleExpression(const IR::Expression *e);
bool isNonConstantSimpleExpression(const IR::Expression *e);
bool isCommutativeBinaryOperation(const IR::Operation_Binary *bin);
bool isStandardMetadata(cstring name);
bool isMetadataStruct(const IR::Type_Struct *st);
bool isMetadataField(const IR::Expression *e);
bool isEightBitAligned(const IR::Expression *e);
bool isDirection(const IR::Member *m);
bool isHeadersStruct(const IR::Type_Struct *st);
bool isLargeFieldOperand(const IR::Expression *e);
bool isInsideHeader(const IR::Expression *e);
bool isValidCall(const IR::MethodCallExpression *m);
bool isValidMemberField(const IR::Member *mem);
int getMetadataFieldWidth(int width);
const IR::Type_Bits *getEightBitAlignedType(const IR::Type_Bits *tb);

/// Check for reserved names for DPDK target.
bool reservedNames(P4::ReferenceMap *refMap, const std::vector<cstring> &names, cstring &resName);
/// Creates Register extern declaration for holding persistent information.
IR::Declaration_Instance *createRegDeclarationInstance(cstring instanceName, int regSize,
                                                       int indexBitWidth, int initValBitwidth);
}  // namespace P4::DPDK
#endif /* BACKENDS_DPDK_DPDKUTILS_H_ */
