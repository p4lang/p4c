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
#include "ir/ir.h"
#include "typeUnification.h"
#include "typeConstraints.h"
#include "typeSubstitution.h"
#include "typeSubstitutionVisitor.h"

namespace P4 {

// A list of equality constraints on types.
class TypeConstraints final {
    class IConstraint {
     public:
        virtual ~IConstraint() {}
        virtual void dbprint(std::ostream& out) const = 0;
    };

    class TwoTypeConstraint : public IConstraint {
     public:
        const IR::Type* left;
        const IR::Type* right;

     protected:
        TwoTypeConstraint(const IR::Type* left, const IR::Type* right) :
                left(left), right(right) {
            CHECK_NULL(left); CHECK_NULL(right);
            if (left->is<IR::Type_Name>() || right->is<IR::Type_Name>())
                BUG("Unifying type names %1% and %2%", left, right);
        }
    };

    // Requires two types to be equal.
    class EqualityConstraint : public TwoTypeConstraint {
     public:
        EqualityConstraint(const IR::Type* left, const IR::Type* right)
                : TwoTypeConstraint(left, right) {}
        void dbprint(std::ostream& out) const override
        { out << "Constraint:" << dbp(left) << " = " << dbp(right); }
    };

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
    std::vector<IConstraint*> constraints;
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
    void addEqualityConstraint(const IR::Type* left, const IR::Type* right);

    /*
     * Solve the specified constraint.
     * @param root       Element where error is signalled if necessary.
     * @param subst      Variable substitution which is updated with new constraints.
     * @param constraint Constraint to solve.
     * @param reportErrors If true report errors.
     * @return           True on success.
     */
    bool solve(const IR::Node* root, IConstraint* constraint,
               TypeVariableSubstitution *subst, bool reportErrors) {
        auto eq = dynamic_cast<EqualityConstraint*>(constraint);
        if (eq != nullptr)
            return solve(root, eq, subst, reportErrors);
        BUG("unexpected type constraint");
    }

    bool solve(const IR::Node* root, EqualityConstraint *constraint,
               TypeVariableSubstitution *subst, bool reportErrors);
    TypeVariableSubstitution* solve(const IR::Node* root, bool reportErrors);
    void dbprint(std::ostream& out) const;
};
}  // namespace P4

#endif /* _TYPECHECKING_TYPECONSTRAINTS_H_ */
