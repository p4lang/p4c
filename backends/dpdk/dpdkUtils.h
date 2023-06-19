/* Copyright 2022 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef BACKENDS_DPDK_DPDKUTILS_H_
#define BACKENDS_DPDK_DPDKUTILS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"

namespace DPDK {
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
int getMetadataFieldWidth(int width);
const IR::Type_Bits *getEightBitAlignedType(const IR::Type_Bits *tb);

// Check for reserved names for DPDK target
bool reservedNames(P4::ReferenceMap *refMap, std::vector<cstring> names, cstring &resName);
// Creates Register extern declaration for holding persistent information
IR::Declaration_Instance *createRegDeclarationInstance(cstring instanceName, int regSize,
                                                       int indexBitWidth, int initValBitwidth);
}  // namespace DPDK
#endif /* BACKENDS_DPDK_DPDKUTILS_H_ */
