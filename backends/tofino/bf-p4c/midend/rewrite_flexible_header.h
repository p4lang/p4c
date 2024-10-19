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
