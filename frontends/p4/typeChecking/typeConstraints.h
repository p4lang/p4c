/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef _TYPECHECKING_TYPECONSTRAINTS_H_
#define _TYPECHECKING_TYPECONSTRAINTS_H_

#include <sstream>
#include <boost/optional.hpp>
#include "ir/ir.h"
#include "typeUnification.h"
#include "typeConstraints.h"
#include "typeSubstitution.h"
#include "typeSubstitutionVisitor.h"
#include "lib/error_helper.h"

namespace P4 {

/// Requires two types to be equal.
class EqualityConstraint : public IHasDbPrint {
 public:
    const IR::Type* left;
    const IR::Type* right;
    /// Constraint which produced this one.  May be nullptr.
    const EqualityConstraint* derivedFrom;
    /// Place in source code which originated the contraint.  May be nullptr.
    const IR::Node* origin;
    EqualityConstraint(const IR::Type* left, const IR::Type* right,
                       const EqualityConstraint* derivedFrom)
            : left(left), right(right), derivedFrom(derivedFrom), origin(nullptr) { check(); }
    EqualityConstraint(const IR::Type* left, const IR::Type* right,
                       const IR::Node* origin)
            : left(left), right(right), derivedFrom(nullptr), origin(origin) { check(); }
    void check() const {
        CHECK_NULL(left); CHECK_NULL(right);
        if (left->is<IR::Type_Name>() || right->is<IR::Type_Name>())
            BUG("Unifying type names %1% and %2%", left, right);
        LOG3(this);
    }
    void dbprint(std::ostream& out) const override
    { out << "Constraint:" << dbp(left) << " = " << dbp(right); }

    template <typename... T>
    void reportError(const char* format, T... args) const {
        boost::format fmt(format);
        auto message = ::error_helper(fmt, "", "", "", args...);
        auto o = origin;
        auto constraint = derivedFrom;
        while (constraint) {
            boost::format fmt("----While unifying '%1%' with '%2%'");
            message += ::error_helper(fmt, "", "", "", constraint->left, constraint->right);
            o = constraint->origin;
            constraint = constraint->derivedFrom;
        }
        CHECK_NULL(o);
        ::error(ErrorType::ERR_TYPE_ERROR, (cstring("%1%\n") + cstring(message).trim()).c_str(), o);
    }
    void reportError() const {
        reportError("Cannot unify %1% to %2%", right, left);
    }
};

// A list of equality constraints on types.
class TypeConstraints final {
 private:
    /*
     * Not all type variables that appear in unification can be bound:
     * only the ones in this set can be.
     *
     * Consider this example:
     *
     * extern void f(in bit x);
     * control p<T>(T data) { f(data); }
     *
     * This example should not typecheck: because T cannot be constrained in the invocation of f.
     * While typechecking the f(data) call, T is not a type variable that can be unified.
     */
    std::set<const IR::ITypeVar*> unifiableTypeVariables;
    std::vector<EqualityConstraint*> constraints;
    TypeUnification *unification;
    const TypeVariableSubstitution* definedVariables;

 public:
    TypeVariableSubstitutionVisitor replaceVariables;

    explicit TypeConstraints(const TypeVariableSubstitution* definedVariables) :
            unification(new TypeUnification(this)), definedVariables(definedVariables),
            replaceVariables(definedVariables) {}

    // Mark this variable as being free.
    void addUnifiableTypeVariable(const IR::ITypeVar* typeVariable)
    { unifiableTypeVariables.insert(typeVariable); }

    /// True if type is a type variable that can be unified.
    /// A variable is unifiable if it is marked so and it not already
    /// part of definedVariables.
    bool isUnifiableTypeVariable(const IR::Type* type);
    void addEqualityConstraint(
        const IR::Type* left, const IR::Type* right, const EqualityConstraint* derivedFrom);
    void addEqualityConstraint(
        const IR::Node* source, const IR::Type* left, const IR::Type* right);

    /*
     * Solve the specified constraint.
     * @param subst      Variable substitution which is updated with new constraints.
     * @param constraint Constraint to solve.
     * @return           True on success.  Does not report error on failure.
     */
    bool solve(EqualityConstraint *constraint, TypeVariableSubstitution *subst);
    TypeVariableSubstitution* solve();
    void dbprint(std::ostream& out) const;
};
}  // namespace P4

#endif /* _TYPECHECKING_TYPECONSTRAINTS_H_ */
