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

/**
 * \defgroup RewriteFlexibleStruct BFN::RewriteFlexibleStruct
 * \ingroup midend
 * \brief Set of passes that moves flexible annotation from structure to its fields.
 */
#ifndef BF_P4C_MIDEND_REWRITE_FLEXIBLE_HEADER_H_
#define BF_P4C_MIDEND_REWRITE_FLEXIBLE_HEADER_H_

#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {
class TypeMap;
class ReferenceMap;
}  // namespace P4

namespace BFN {

/**
 * \ingroup RewriteFlexibleStruct
 */
struct RewriteHeader : public Transform {
 public:
    RewriteHeader() {}
    const IR::Node *postorder(IR::Type_Struct *st) override {
        if (st->getAnnotation("flexible"_cs) == nullptr) return st;

        IR::IndexedVector<IR::StructField> fields;
        for (auto f : st->fields) {
            auto *fieldAnnotations = new IR::Annotations();
            fieldAnnotations->annotations.push_back(new IR::Annotation(IR::ID("flexible"), {}));
            fields.push_back(new IR::StructField(f->name, fieldAnnotations, f->type));
        }

        return new IR::Type_Struct(st->name, st->annotations, fields);
    }
};

/**
 * \ingroup RewriteFlexibleStruct
 * \brief Top level PassManager that governs moving of flexible annotation
 *        from structure to its fields.
 */
class RewriteFlexibleStruct : public PassManager {
 public:
    explicit RewriteFlexibleStruct(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
        addPasses({
            new RewriteHeader(),
            new P4::ClearTypeMap(typeMap),
            new BFN::TypeChecking(refMap, typeMap, true),
        });
    }
};

}  // namespace BFN

#endif /* BF_P4C_MIDEND_REWRITE_FLEXIBLE_HEADER_H_ */
