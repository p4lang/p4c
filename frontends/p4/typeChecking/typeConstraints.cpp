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

int TypeConstraint::crtid = 0;

void TypeConstraints::addEqualityConstraint(
    const IR::Node* source, const IR::Type* left, const IR::Type* right) {
    auto c = new EqualityConstraint(left, right, source);
    add(c);
}

TypeVariableSubstitution* TypeConstraints::solve() {
    LOG3("Solving constraints:\n" << *this);
    currentSubstitution = new TypeVariableSubstitution();
    while (!constraints.empty()) {
        auto last = constraints.back();
        constraints.pop_back();
        bool success = solve(dynamic_cast<const EqualityConstraint*>(last));
        if (!success)
            return nullptr;
    }
    LOG3("Constraint solution:\n" << currentSubstitution);
    return currentSubstitution;
}

bool TypeConstraints::isUnifiableTypeVariable(const IR::Type* type) {
    auto tv = type->to<IR::ITypeVar>();
    if (tv == nullptr)
        return false;
    if (definedVariables->containsKey(tv))
        return false;
    if (tv->is<IR::Type_InfInt>())
        // These are always unifiable
        return true;
    return unifiableTypeVariables.count(tv) > 0;
}

bool TypeConstraints::solve(const EqualityConstraint *constraint) {
    if (isUnifiableTypeVariable(constraint->left)) {
        auto leftTv = constraint->left->to<IR::ITypeVar>();
        if (constraint->left == constraint->right)
            return true;

        // check to see whether we already have a substitution for leftTv
        const IR::Type* leftSubst = currentSubstitution->lookup(leftTv);
        if (leftSubst == nullptr) {
            auto right = constraint->right->apply(replaceVariables)->to<IR::Type>();
            if (leftTv == right->to<IR::ITypeVar>())
                return true;
            LOG3("Binding " << leftTv << " => " << right);
            auto error = currentSubstitution->compose(leftTv, right);
            if (!error.isNullOrEmpty())
                return constraint->reportError(getCurrentSubstitution(), error, leftTv, right);
            return true;
        } else {
            add(constraint->create(leftSubst, constraint->right));
            return true;
        }
    }

    if (isUnifiableTypeVariable(constraint->right)) {
        auto rightTv = constraint->right->to<IR::ITypeVar>();
        const IR::Type* rightSubst = currentSubstitution->lookup(rightTv);
        if (rightSubst == nullptr) {
            auto left = constraint->left->apply(replaceVariables)->to<IR::Type>();
            if (left->to<IR::ITypeVar>() == rightTv)
                return true;
            LOG3("Binding " << rightTv << " => " << left);
            auto error = currentSubstitution->compose(rightTv, left);
            if (!error.isNullOrEmpty())
                return constraint->reportError(getCurrentSubstitution(), error, rightTv, left);
            return true;
        } else {
            add(constraint->create(constraint->left, rightSubst));
            return true;
        }
    }

    return unification->unify(constraint);
    // this may add more constraints
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
