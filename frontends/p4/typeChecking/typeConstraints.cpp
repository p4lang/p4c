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

#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "typeUnification.h"

namespace P4 {

int TypeConstraint::crtid = 0;

void TypeConstraints::addEqualityConstraint(const IR::Node *source, const IR::Type *left,
                                            const IR::Type *right) {
    auto c = new EqualityConstraint(left, right, source);
    add(c);
}

void TypeConstraints::addImplicitCastConstraint(const IR::Node *source, const IR::Type *left,
                                                const IR::Type *right) {
    auto c = new CanBeImplicitlyCastConstraint(left, right, source);
    add(c);
}

TypeVariableSubstitution *TypeConstraints::solve() {
    LOG3("Solving constraints:\n" << *this);
    currentSubstitution = new TypeVariableSubstitution();
    while (!constraints.empty()) {
        auto last = constraints.back();
        constraints.pop_back();
        bool success = solve(last->to<P4::BinaryConstraint>());
        if (!success) return nullptr;
    }
    LOG3("Constraint solution:\n" << currentSubstitution);
    return currentSubstitution;
}

bool TypeConstraints::isUnifiableTypeVariable(const IR::Type *type) {
    auto tv = type->to<IR::ITypeVar>();
    if (tv == nullptr) return false;
    if (definedVariables->containsKey(tv)) return false;
    if (tv->is<IR::Type_InfInt>() || tv->is<IR::Type_Any>())
        // These are always unifiable
        return true;
    return unifiableTypeVariables.count(tv) > 0;
}

bool TypeConstraints::solve(const BinaryConstraint *constraint) {
    if (isUnifiableTypeVariable(constraint->left) &&
        !constraint->right->is<IR::Type_Any>()) {  // we prefer to substitute ANY
        auto leftTv = constraint->left->to<IR::ITypeVar>();
        if (constraint->left == constraint->right) return true;

        // check to see whether we already have a substitution for leftTv
        const IR::Type *leftSubst = currentSubstitution->lookup(leftTv);
        if (leftSubst == nullptr) {
            auto right = constraint->right->apply(replaceVariables)->to<IR::Type>();
            if (leftTv == right->to<IR::ITypeVar>()) return true;
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
        const IR::Type *rightSubst = currentSubstitution->lookup(rightTv);
        if (rightSubst == nullptr) {
            auto left = constraint->left->apply(replaceVariables)->to<IR::Type>();
            if (left->to<IR::ITypeVar>() == rightTv) return true;
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

void TypeConstraints::dbprint(std::ostream &out) const {
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
        out << std::endl << "Constraints: ";
        for (auto c : constraints) out << std::endl << c;
    }
}

std::string TypeConstraint::localError(Explain *explainer) const {
    if (errFormat.isNullOrEmpty()) return "";

    std::string message, explanation;
    boost::format fmt = boost::format(errFormat);
    switch (errArguments.size()) {
        case 0:
            message = boost::str(fmt);
            break;
        case 1:
            absl::StrAppend(&explanation, explain(0, explainer));
            message = ::error_helper(fmt, errArguments.at(0)).toString();
            break;
        case 2:
            absl::StrAppend(&explanation, explain(0, explainer), explain(1, explainer));
            message = ::error_helper(fmt, errArguments.at(0), errArguments.at(1)).toString();
            break;
        case 3:
            absl::StrAppend(&explanation, explain(0, explainer), explain(1, explainer),
                            explain(2, explainer));
            message =
                ::error_helper(fmt, errArguments.at(0), errArguments.at(1), errArguments.at(2))
                    .toString();
            break;
        case 4:
            absl::StrAppend(&explanation, explain(0, explainer), explain(1, explainer),
                            explain(2, explainer), explain(3, explainer));
            message = ::error_helper(fmt, errArguments.at(0), errArguments.at(1),
                                     errArguments.at(2), errArguments.at(3))
                          .toString();
            break;
        default:
            BUG("Unexpected argument count for error message");
    }
    return absl::StrCat(message, explanation);
}

bool TypeConstraint::reportErrorImpl(const TypeVariableSubstitution *subst,
                                     std::string message) const {
    const auto *o = origin;
    const auto *constraint = this;

    Explain explainer(subst);
    while (constraint) {
        std::string local = constraint->localError(&explainer);
        if (!local.empty()) absl::StrAppend(&message, "---- Originating from:\n", local);
        o = constraint->origin;
        constraint = constraint->derivedFrom;
    }

    // Indent each string in the message
    std::vector<std::string_view> lines = absl::StrSplit(message, '\n');
    bool lastIsEmpty = lines.back().empty();
    if (lastIsEmpty)
        // We don't want to indent an empty line.
        lines.pop_back();
    message = absl::StrJoin(lines, "\n  ");
    if (lastIsEmpty) message += "\n";

    CHECK_NULL(o);
    ::errorWithSuffix(ErrorType::ERR_TYPE_ERROR, "'%1%'", message.c_str(), o);
    return false;
}

}  // namespace P4
