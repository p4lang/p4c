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

#include "typeConstraints.h"
#include "typeUnification.h"

namespace P4 {

bool TypeConstraints::solve(const IR::Node* root, EqualityConstraint *constraint,
                            TypeVariableSubstitution *subst, bool reportErrors) {
    if (isUnifiableTypeVariable(constraint->left)) {
        auto leftTv = constraint->left->to<IR::ITypeVar>();
        if (constraint->left == constraint->right)
            return true;

        // check to see whether we already have a substitution for leftTv
        const IR::Type* leftSubst = subst->lookup(leftTv);
        if (leftSubst == nullptr) {
            LOG3("Binding " << leftTv << " => " << constraint->right);
            return subst->compose(root, leftTv, constraint->right);
        } else {
            addEqualityConstraint(leftSubst, constraint->right);
            return true;
        }
    }

    if (isUnifiableTypeVariable(constraint->right)) {
        auto rightTv = constraint->right->to<IR::ITypeVar>();
        const IR::Type* rightSubst = subst->lookup(rightTv);
        if (rightSubst == nullptr) {
            LOG3("Binding " << rightTv << " => " << constraint->left);
            return subst->compose(root, rightTv, constraint->left);
        } else {
            addEqualityConstraint(constraint->left, rightSubst);
            return true;
        }
    }

    bool success = unification->unify(root, constraint->left, constraint->right, reportErrors);
    // this may add more constraints
    return success;
}

void TypeConstraints::dbprint(std::ostream& out) const {
    bool first = true;
    if (unifiableTypeVariables.size() != 0) {
        out << "Variables: ";
        for (auto tv : unifiableTypeVariables) {
            if (!first) out << ", ";
            auto node = tv->getNode();
            out << dbp(node);
            first = false;
        }
    }
    if (constraints.size() != 0) {
        out << "Constraints: ";
        for (auto c : constraints)
            out << std::endl << c;
    }
}

}  // namespace P4
