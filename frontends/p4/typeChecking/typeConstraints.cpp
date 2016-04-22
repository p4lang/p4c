#include "typeConstraints.h"
#include "typeUnification.h"

namespace P4 {

bool TypeConstraints::solve(const IR::Node* root, EqualityConstraint *constraint,
                            IR::TypeVariableSubstitution *subst, bool reportErrors) {
    if (isUnifiableTypeVariable(constraint->left)) {
        auto leftTv = constraint->left->to<IR::ITypeVar>();
        if (constraint->left == constraint->right)
            return true;

        // check to see whether we already have a substitution for leftTv
        const IR::Type* leftSubst = subst->lookup(leftTv);
        if (leftSubst == nullptr) {
            LOG1("Binding " << leftTv << " => " << constraint->right);
            return subst->compose(leftTv, constraint->right);
        } else {
            addEqualityConstraint(leftSubst, constraint->right);
            return true;
        }
    }

    if (isUnifiableTypeVariable(constraint->right)) {
        auto rightTv = constraint->right->to<IR::ITypeVar>();
        const IR::Type* rightSubst = subst->lookup(rightTv);
        if (rightSubst == nullptr) {
            LOG1("Binding " << rightTv << " => " << constraint->left);
            return subst->compose(rightTv, constraint->left);
        } else {
            addEqualityConstraint(constraint->left, rightSubst);
            return true;
        }
    }

    bool success = unification->unify(root, constraint->left, constraint->right, reportErrors);
    // this may add more constraints
    return success;
}

}  // namespace P4
