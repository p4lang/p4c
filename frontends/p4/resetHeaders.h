/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_RESETHEADERS_H_
#define FRONTENDS_P4_RESETHEADERS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/** @brief Explicitly invalidate uninitialized header variables.
 *
 * A local uninitialized variable that represents a header
 * must be initialized to invalid.  For example:
 *
```
state X {
    H h;
    p.extract(h);
}
```
 *
 * becomes
 *
```
state X {
    H h;
    h.setInvalid();
    p.extract(h);
}
```
 *
 * This pass also handles header fields in variables of derived types, like
 * structs and unions.
 *
 * @pre An up-to-date TypeMap.
 *
 * @post Uninitialized header variables have explicit
 * statements that invalidate those headers.
 */
class DoResetHeaders : public Transform {
    const TypeMap *typeMap;
    IR::IndexedVector<IR::StatOrDecl> insert;

 public:
    static void generateResets(const TypeMap *typeMap, const IR::Type *type,
                               const IR::Expression *expr, IR::Vector<IR::StatOrDecl> *resets);
    explicit DoResetHeaders(const TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("DoResetHeaders");
    }
    const IR::Node *postorder(IR::Declaration_Variable *decl) override;
    const IR::Node *postorder(IR::P4Control *control) override;
    const IR::Node *postorder(IR::ParserState *state) override;
};

/// Invokes TypeChecking followed by DoResetHeaders.
class ResetHeaders : public PassManager {
 public:
    explicit ResetHeaders(TypeMap *typeMap) {
        passes.push_back(new P4::TypeChecking(nullptr, typeMap));
        passes.push_back(new P4::DoResetHeaders(typeMap));
        setName("ResetHeaders");
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_RESETHEADERS_H_ */
