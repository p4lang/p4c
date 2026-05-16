/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_TABLEKEYNAMES_H_
#define FRONTENDS_P4_TABLEKEYNAMES_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/** Generate control plane names for simple expressions that appear in table
 * keys without `@name` annotations.
 *
 * Emit a compilation error if the IR contains complex expressions without
 * `@name` annotations.
 */
class KeyNameGenerator : public Inspector {
    std::map<const IR::Expression *, cstring> name;
    const TypeMap *typeMap;  // can be nullptr

 public:
    explicit KeyNameGenerator(const TypeMap *typeMap) : typeMap(typeMap) {
        setName("KeyNameGenerator");
    }
    void error(const IR::Expression *expression);
    void postorder(const IR::Expression *expression) override;
    void postorder(const IR::PathExpression *expression) override;
    void postorder(const IR::Member *expression) override;
    void postorder(const IR::ArrayIndex *expression) override;
    void postorder(const IR::Constant *expression) override;
    void postorder(const IR::Slice *expression) override;
    void postorder(const IR::BAnd *expression) override;
    void postorder(const IR::MethodCallExpression *expression) override;
    cstring getName(const IR::Expression *expression) { return ::P4::get(name, expression); }
};

/** Adds a "@name" annotation to each table key that does not have a name.
 * The string used for the name is derived from the expression itself - if
 * the expression is "simple" enough.  If the expression is not simple the
 * compiler will give an error.  Simple expressions are:
 * - .isValid(),
 * - ArrayIndex,
 * - Constant,
 * - Member,
 * - PathExpression,
 * - Slice.
 *
 * Examples of control plane names generated from expressions:
 * - `arr[16w5].f` : `@name("arr[5].f")`
 * - `.foo` : `@name(".foo")`
 * - `foo.bar` : `@name("foo.bar")`
 * - `f.isValid()` : `@name("f.isValid()")`
 * - `f[3:0]` : `@name("f[3:0]")`
 *
 * @pre This must run before passes that change key expressions, eg. constant
 * folding.  Otherwise the generated control plane names may not match the
 * syntax of the original P4 program.
 *
 * @post All key fields have `@name` annotations.
 *
 * Emit a compilation error if the program contains complex key expressions
 * without `@name` annotations.
 */
class DoTableKeyNames final : public Transform {
    const TypeMap *typeMap;

 public:
    explicit DoTableKeyNames(const TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("DoTableKeyNames");
    }
    const IR::Node *postorder(IR::KeyElement *keyElement) override;
};

class TableKeyNames final : public PassManager {
 public:
    explicit TableKeyNames(TypeMap *typeMap) {
        passes.push_back(new TypeChecking(nullptr, typeMap));
        passes.push_back(new DoTableKeyNames(typeMap));
        setName("TableKeyNames");
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_TABLEKEYNAMES_H_ */
