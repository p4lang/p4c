/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BF_P4C_ARCH_HELPERS_H_
#define BF_P4C_ARCH_HELPERS_H_

#include <optional>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/externInstance.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace BFN {

const IR::Declaration_Instance *getDeclInst(const P4::ReferenceMap *refMap,
                                            const IR::PathExpression *path);

std::optional<cstring> getExternTypeName(const P4::ExternMethod *extMethod);

std::optional<P4::ExternInstance> getExternInstanceFromPropertyByTypeName(
    const IR::P4Table *table, cstring propertyName, cstring typeName, P4::ReferenceMap *refMap,
    P4::TypeMap *typeMap, bool *isConstructedInPlace = nullptr);

/// @return an extern instance defined or referenced by the value of @p table's
/// @p propertyName property, or std::nullopt if no extern was referenced.
std::optional<P4::ExternInstance> getExternInstanceFromProperty(const IR::P4Table *table,
                                                                cstring propertyName,
                                                                P4::ReferenceMap *refMap,
                                                                P4::TypeMap *typeMap);

std::optional<const IR::ExpressionValue *> getExpressionFromProperty(const IR::P4Table *table,
                                                                     const cstring &propertyName);

std::vector<const IR::Expression *> convertConcatToList(const IR::Concat *expr);

}  // namespace BFN

#endif /* BF_P4C_ARCH_HELPERS_H_ */
