/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_EBPF_LOWER_H_
#define BACKENDS_EBPF_LOWER_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4::EBPF {

/// This pass rewrites expressions which are not supported natively on EBPF.
class LowerExpressions : public Transform {
    P4::TypeMap *typeMap;
    std::optional<int> maxShiftWidth;
    const IR::Expression *shift(const IR::Operation_Binary *expression) const;

 public:
    explicit LowerExpressions(P4::TypeMap *typeMap, std::optional<int> maxShiftWidth)
        : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        this->maxShiftWidth = maxShiftWidth;
        setName("LowerExpressions");
    }

    const IR::Node *postorder(IR::Shl *expression) override { return shift(expression); }
    const IR::Node *postorder(IR::Shr *expression) override { return shift(expression); }
    const IR::Node *postorder(IR::Expression *expression) override;
    const IR::Node *postorder(IR::Slice *expression) override;
    const IR::Node *postorder(IR::Concat *expression) override;
    const IR::Node *postorder(IR::Cast *expression) override;
};

class Lower : public PassManager {
 public:
    Lower(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, std::optional<int> maxShiftWidth) {
        setName("Lower");
        passes.push_back(new P4::TypeChecking(refMap, typeMap));
        passes.push_back(new LowerExpressions(typeMap, maxShiftWidth));
    }
};

}  // namespace P4::EBPF

#endif /* BACKENDS_EBPF_LOWER_H_ */
