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

#ifndef TYPECHECKING_TYPECONSTRAINTS_H_
#define TYPECHECKING_TYPECONSTRAINTS_H_

#include <optional>
#include <sstream>

#include <boost/algorithm/string.hpp>

#include "ir/ir.h"
#include "lib/castable.h"
#include "lib/error_helper.h"
#include "typeConstraints.h"
#include "typeSubstitution.h"
#include "typeSubstitutionVisitor.h"
#include "typeUnification.h"

namespace P4 {

/// Creates a string that describes the values of current type variables
class Explain : public Inspector {
    std::set<const IR::ITypeVar *> explained;
    const TypeVariableSubstitution *subst;

 public:
    std::string explanation;
    explicit Explain(const TypeVariableSubstitution *subst) : subst(subst) { CHECK_NULL(subst); }
    profile_t init_apply(const IR::Node *node) override {
        explanation = "";
        return Inspector::init_apply(node);
    }
    void postorder(const IR::Type_Var *tv) override {
        if (explained.find(tv) != explained.end())
            // Do not repeat explanations.
            return;
        explained.emplace(tv);
        auto val = subst->lookup(tv);
        if (!val) return;
        explanation += "Where '" + tv->toString() + "' is bound to '" + val->toString() + "'\n";
        Explain erec(subst);  // recursive explain variables in this substitution
        erec.setCalledBy(this);
        val->apply(erec);
        explanation += erec.explanation;
    }
};

class TypeConstraint : public IHasDbPrint, public ICastable {
    int id;  // for debugging
    static int crtid;
    /// The following are used when reporting errors.
    cstring errFormat;
    std::vector<const IR::Node *> errArguments;

 protected:
    /// Constraint which produced this one.  May be nullptr.
    const TypeConstraint *derivedFrom = nullptr;
    /// Place in source code which originated the contraint.  May be nullptr.
    const IR::Node *origin = nullptr;

    explicit TypeConstraint(const TypeConstraint *derivedFrom)
        : id(crtid++), derivedFrom(derivedFrom) {}
    explicit TypeConstraint(const IR::Node *origin) : id(crtid++), origin(origin) {}
    std::string explain(size_t index, Explain *explainer) const {
        auto node = errArguments.at(index);
        node->apply(*explainer);
        return explainer->explanation;
    }
    cstring localError(Explain *explainer) const {
        if (errFormat.isNullOrEmpty()) return "";
        std::string message, explanation;
        boost::format fmt = boost::format(errFormat);
        switch (errArguments.size()) {
            case 0:
                message = boost::str(fmt);
                break;
            case 1:
                explanation += explain(0, explainer);
                message = ::error_helper(fmt, errArguments.at(0)).toString();
                break;
            case 2:
                explanation += explain(0, explainer);
                explanation += explain(1, explainer);
                message = ::error_helper(fmt, errArguments.at(0), errArguments.at(1)).toString();
                break;
            case 3:
                explanation += explain(0, explainer);
                explanation += explain(1, explainer);
                explanation += explain(2, explainer);
                message =
                    ::error_helper(fmt, errArguments.at(0), errArguments.at(1), errArguments.at(2))
                        .toString();
                break;
            case 4:
                explanation += explain(0, explainer);
                explanation += explain(1, explainer);
                explanation += explain(2, explainer);
                explanation += explain(3, explainer);
                message = ::error_helper(fmt, errArguments.at(0), errArguments.at(1),
                                         errArguments.at(2), errArguments.at(3))
                              .toString();
                break;
            default:
                BUG("Unexpected argument count for error message");
        }
        return cstring(message) + explanation;
    }

 public:
    void setError(cstring format, std::initializer_list<const IR::Node *> nodes) {
        errFormat = format;
        errArguments = nodes;
    }
    template <typename... T>
    // Always return false.
    bool reportError(const TypeVariableSubstitution *subst, const char *format, T... args) const {
        /// The Constraints actually form a stack and the error message
        /// is composed in reverse order, from bottom to top.
        /// The top of the stack has no 'derivedFrom' field,
        /// and it contains the actual source position where
        /// the analysis started.
        boost::format fmt(format);
        cstring message =
            cstring("  ---- Actual error:\n") + ::error_helper(fmt, args...).toString();
        auto o = origin;
        auto constraint = this;
        Explain explainer(subst);
        while (constraint) {
            cstring local = constraint->localError(&explainer);
            if (!local.isNullOrEmpty()) {
                message += "---- Originating from:\n";
                message += local;
            }
            o = constraint->origin;
            constraint = constraint->derivedFrom;
        }
        // Indent each string in the message
        std::string s = message.c_str();
        std::vector<std::string> lines;
        boost::split(lines, s, [](char c) { return c == '\n'; });
        bool lastIsEmpty = lines.at(lines.size() - 1) == "";
        if (lastIsEmpty)
            // We don't want to indent an empty line.
            lines.pop_back();
        message = cstring::join(lines.begin(), lines.end(), "\n  ");
        if (lastIsEmpty) message += "\n";

        CHECK_NULL(o);
        ::errorWithSuffix(ErrorType::ERR_TYPE_ERROR, "%1%", message.c_str(), o);
        return false;
    }
    // Default error message; returns 'false'
    virtual bool reportError(const TypeVariableSubstitution *subst) const = 0;
};

/// Base class for EqualityConstraint and CanBeImplicitlyCastConstraint
class BinaryConstraint : public TypeConstraint {
 public:
    const IR::Type *left;
    const IR::Type *right;

 protected:
    BinaryConstraint(const IR::Type *left, const IR::Type *right, const TypeConstraint *derivedFrom)
        : TypeConstraint(derivedFrom), left(left), right(right) {
        validate();
    }
    BinaryConstraint(const IR::Type *left, const IR::Type *right, const IR::Node *origin)
        : TypeConstraint(origin), left(left), right(right) {
        validate();
    }
    void validate() const {
        CHECK_NULL(left);
        CHECK_NULL(right);
        if (left->is<IR::Type_Name>() || right->is<IR::Type_Name>())
            BUG("type names should not appear in unification: %1% and %2%", left, right);
    }

 public:
    virtual void dbprint(std::ostream &out) const = 0;
    virtual BinaryConstraint *create(const IR::Type *left, const IR::Type *right) const = 0;
};

/// Requires two types to be equal.
class EqualityConstraint : public BinaryConstraint {
 public:
    EqualityConstraint(const IR::Type *left, const IR::Type *right,
                       const TypeConstraint *derivedFrom)
        : BinaryConstraint(left, right, derivedFrom) {}
    EqualityConstraint(const IR::Type *left, const IR::Type *right, const IR::Node *origin)
        : BinaryConstraint(left, right, origin) {}
    void dbprint(std::ostream &out) const override {
        out << "Constraint:" << dbp(left) << " == " << dbp(right);
    }
    using TypeConstraint::reportError;
    bool reportError(const TypeVariableSubstitution *subst) const override {
        return reportError(subst, "Cannot unify type '%1%' with type '%2%'", right, left);
    }
    BinaryConstraint *create(const IR::Type *left, const IR::Type *right) const override {
        return new EqualityConstraint(left, right, this);
    }
};

/// The right type can be implicitly cast to the left type.
class CanBeImplicitlyCastConstraint : public BinaryConstraint {
 public:
    CanBeImplicitlyCastConstraint(const IR::Type *left, const IR::Type *right,
                                  const TypeConstraint *derivedFrom)
        : BinaryConstraint(left, right, derivedFrom) {}
    CanBeImplicitlyCastConstraint(const IR::Type *left, const IR::Type *right,
                                  const IR::Node *origin)
        : BinaryConstraint(left, right, origin) {}
    void dbprint(std::ostream &out) const override {
        out << "Constraint:" << dbp(left) << " := " << dbp(right);
    }
    using TypeConstraint::reportError;
    bool reportError(const TypeVariableSubstitution *subst) const override {
        return reportError(subst, "Cannot cast implicitly type '%1%' to type '%2%'", right, left);
    }
    BinaryConstraint *create(const IR::Type *left, const IR::Type *right) const override {
        return new CanBeImplicitlyCastConstraint(left, right, this);
    }
};

// A list of equality constraints on types.
class TypeConstraints final : public IHasDbPrint {
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
    std::set<const IR::ITypeVar *> unifiableTypeVariables;
    std::vector<const TypeConstraint *> constraints;
    TypeUnification *unification;
    const TypeVariableSubstitution *definedVariables;
    /// Keeps track of the values of all variables.
    TypeVariableSubstitution *currentSubstitution;

 public:
    TypeVariableSubstitutionVisitor replaceVariables;

    TypeConstraints(const TypeVariableSubstitution *definedVariables, const P4::TypeMap *typeMap)
        : unification(new TypeUnification(this, typeMap)),
          definedVariables(definedVariables),
          currentSubstitution(new TypeVariableSubstitution()),
          replaceVariables(definedVariables) {}
    // Mark this variable as being free.
    void addUnifiableTypeVariable(const IR::ITypeVar *typeVariable) {
        LOG3("Adding unifiable type variable " << typeVariable);
        unifiableTypeVariables.insert(typeVariable);
    }

    /// True if type is a type variable that can be unified.
    /// A variable is unifiable if it is marked so and it not already
    /// part of definedVariables.
    bool isUnifiableTypeVariable(const IR::Type *type);
    void add(const TypeConstraint *constraint) {
        LOG3("Adding constraint " << constraint);
        constraints.push_back(constraint);
    }
    void addEqualityConstraint(const IR::Node *source, const IR::Type *left, const IR::Type *right);
    /*
     * Solve the specified constraint.
     * @param subst      Variable substitution which is updated with new constraints.
     * @param constraint Constraint to solve.
     * @return           True on success.  Does not report error on failure.
     */
    bool solve(const BinaryConstraint *constraint);
    TypeVariableSubstitution *solve();
    void dbprint(std::ostream &out) const;
    const TypeVariableSubstitution *getCurrentSubstitution() const { return currentSubstitution; }
};
}  // namespace P4

#endif /* TYPECHECKING_TYPECONSTRAINTS_H_ */
