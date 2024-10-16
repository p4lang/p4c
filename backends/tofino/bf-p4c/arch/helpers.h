/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BF_P4C_ARCH_HELPERS_H_
#define BF_P4C_ARCH_HELPERS_H_

#include <optional>

#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/externInstance.h"
#include "frontends/p4/methodInstance.h"

namespace BFN {

const IR::Declaration_Instance *
getDeclInst(const P4::ReferenceMap *refMap, const IR::PathExpression *path);

std::optional<cstring> getExternTypeName(const P4::ExternMethod* extMethod);

std::optional<P4::ExternInstance>
getExternInstanceFromPropertyByTypeName(const IR::P4Table* table,
                                        cstring propertyName,
                                        cstring typeName,
                                        P4::ReferenceMap* refMap,
                                        P4::TypeMap* typeMap,
                                        bool *isConstructedInPlace = nullptr);

/// @return an extern instance defined or referenced by the value of @p table's
/// @p propertyName property, or std::nullopt if no extern was referenced.
std::optional<P4::ExternInstance>
getExternInstanceFromProperty(const IR::P4Table* table,
                              cstring propertyName,
                              P4::ReferenceMap* refMap,
                              P4::TypeMap* typeMap);

std::optional<const IR::ExpressionValue*>
getExpressionFromProperty(const IR::P4Table* table,
                          const cstring& propertyName);

std::vector<const IR::Expression*>
convertConcatToList(const IR::Concat* expr);

}  // namespace BFN

#endif /* BF_P4C_ARCH_HELPERS_H_ */
