#ifndef _TYPECHECKING_TYPECONSTRAINTS_H_
#define _TYPECHECKING_TYPECONSTRAINTS_H_

#include <sstream>
#include "ir/ir.h"
#include "typeUnification.h"
#include "typeConstraints.h"
#include "ir/substitution.h"

namespace P4 {

// A list of equality constraints on types.
class TypeConstraints final {
    class IConstraint {
     public:
        virtual ~IConstraint() {}
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
        void dbprint(std::ostream& out) const
        { out << "Constraint:" << left << " = " << right; }
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

 public:
    TypeConstraints() : unification(new TypeUnification(this)) {}

    void addUnifiableTypeVariable(const IR::ITypeVar* typeVariable)
    { unifiableTypeVariables.insert(typeVariable); }

    // True if type is a type variable that can be unified.
    bool isUnifiableTypeVariable(const IR::Type* type) {
        auto tv = type->to<IR::ITypeVar>();
        if (tv == nullptr)
            return false;
        if (tv->is<IR::Type_InfInt>())
            // These are always unifiable
            return true;
        return unifiableTypeVariables.count(tv) > 0;
    }

    void addEqualityConstraint(const IR::Type* left, const IR::Type* right) {
        auto c = new EqualityConstraint(left, right);
        LOG1(c);
        constraints.push_back(c);
    }

    /*
     * Solve the specified constraint.
     * @param root       Element where error is signalled if necessary.
     * @param subst      Variable substitution which is updated with new constraints.
     * @param constraint Constraint to solve.
     * @param reportErrors If true report errors.
     * @return           True on success.
     */
    bool solve(const IR::Node* root, IConstraint* constraint,
               IR::TypeVariableSubstitution *subst, bool reportErrors) {
        auto eq = dynamic_cast<EqualityConstraint*>(constraint);
        if (eq != nullptr)
            return solve(root, eq, subst, reportErrors);
        BUG("unexpected type constraint");
    }

    bool solve(const IR::Node* root, EqualityConstraint *constraint,
               IR::TypeVariableSubstitution *subst, bool reportErrors);

    IR::TypeVariableSubstitution* solve(const IR::Node* root, bool reportErrors) {
        LOG1("Solving constraints:\n" << this);

        IR::TypeVariableSubstitution *tvs = new IR::TypeVariableSubstitution();
        while (!constraints.empty()) {
            auto last = constraints.back();
            constraints.pop_back();
            bool success = solve(root, last, tvs, reportErrors);
            if (!success)
                return nullptr;
        }
        LOG1("Constraint solution:\n" << tvs);
        return tvs;
    }

    void dbprint(std::ostream& out) const {
        bool first = true;
        out << "Variables: ";
        for (auto tv : unifiableTypeVariables) {
            if (!first) out << ", ";
            out << tv->getNode();
            first = false;
        }
        for (auto c : constraints)
            out << std::endl << c;
    }
};
}  // namespace P4

#endif /* _TYPECHECKING_TYPECONSTRAINTS_H_ */
