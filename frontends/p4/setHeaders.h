/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_SETHEADERS_H_
#define FRONTENDS_P4_SETHEADERS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {
/**
Assigning a list expression to a header should also
set the header validity bit.  For example, given:

header H { ... };
struct S { H h; }
S s;

The following fragment:

s = { { 1, 2 } };

is converted to:

s.h.setValid();
s = { { 1, 2 } };

@pre This pass should be run after RemoveInitializers - it only looks
at assignment statements.  It should also run after
SideEffectOrdering, because that pass inserts temporaries for the case
of tuples passed as arguments to functions expecting headers, reducing
them to assignments.
*/
class DoSetHeaders final : public Transform {
    TypeMap *typeMap;

    bool containsHeaderType(const IR::Type *type);
    void generateSetValid(const IR::Expression *dest, const IR::Expression *src,
                          const IR::Type *destType, IR::Vector<IR::StatOrDecl> &insert);

 public:
    explicit DoSetHeaders(TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("DoSetHeaders");
    }
    const IR::Node *postorder(IR::AssignmentStatement *assign) override;
};

class SetHeaders final : public PassManager {
 public:
    explicit SetHeaders(TypeMap *typeMap) {
        passes.push_back(new P4::TypeChecking(nullptr, typeMap));
        passes.push_back(new P4::DoSetHeaders(typeMap));
        setName("SetHeaders");
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_SETHEADERS_H_ */
