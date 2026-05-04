/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_MOVECONSTRUCTORS_H_
#define FRONTENDS_P4_MOVECONSTRUCTORS_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/ordered_map.h"

namespace P4 {

/**
 * This pass converts constructor call expressions that appear
 * within the bodies of P4Parser and P4Control blocks
 * into Declaration_Instance. This is needed for implementing
 * copy-in/copy-out in inlining, since
 * constructed objects do not have assignment operations.
 *
 * For example:
 * extern T {}
 * control c()(T t) {  apply { ... } }
 * control d() {
 *    c(T()) cinst;
 *    apply { ... }
 * }
 * is converted to
 * extern T {}
 * control c()(T t) {  apply { ... } }
 * control d() {
 *    T() tmp;
 *    c(tmp) cinst;
 *    apply { ... }
 * }
 *
 * @pre None
 * @post No constructor call expression in P4Parser and P4Control
 *       constructor parameters.
 *
 */
class MoveConstructors : public Transform {
    struct ConstructorMap {
        // Maps a constructor to the temporary used to hold its value.
        ordered_map<const IR::ConstructorCallExpression *, cstring> tmpName;

        void clear() { tmpName.clear(); }
        void add(const IR::ConstructorCallExpression *expression, cstring name) {
            CHECK_NULL(expression);
            tmpName[expression] = name;
        }
        bool empty() const { return tmpName.empty(); }
    };

    enum class Region { InParserStateful, InControlStateful, InBody, Outside };

    MinimalNameGenerator nameGen;
    ConstructorMap cmap;
    Region convert;

 public:
    MoveConstructors();

    profile_t init_apply(const IR::Node *node) override;
    const IR::Node *preorder(IR::P4Parser *parser) override;
    const IR::Node *preorder(IR::IndexedVector<IR::Declaration> *declarations) override;
    const IR::Node *postorder(IR::P4Parser *parser) override;
    const IR::Node *preorder(IR::P4Control *control) override;
    const IR::Node *postorder(IR::P4Control *control) override;
    const IR::Node *preorder(IR::P4Table *table) override;
    const IR::Node *postorder(IR::ConstructorCallExpression *expression) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_MOVECONSTRUCTORS_H_ */
