#include "substitution.h"
#include "substitutionVisitor.h"

namespace IR {
bool TypeVariableSubstitution::compose(const IR::Type_VarBase* var, const IR::Type* substitution) {
    LOG1("Adding " << var->toString() << "->" << substitution->toString() << "to substitution");

    // First check whether the substitution is legal.
    // It is not if var occurs in substitution
    TypeOccursVisitor occurs(var);
    substitution->apply(occurs);
    if (occurs.occurs)
        return false;

    // Check to see whether we already have a binding for this variable
    if (containsKey(var)) {
        const IR::Type* bound = lookup(var);
        BUG("Two constraints on the same variable %1%: %2% and %3%",
                                var->toString(), substitution->toString(), bound->toString());
    }

    // Replace var with substitution everywhere
    TypeVariableSubstitution *tvs = new TypeVariableSubstitution();
    bool success = tvs->setBinding(var, substitution);
    if (!success)
        BUG("Cannot set binding");

    TypeVariableSubstitutionVisitor visitor(tvs);
    for (auto bound : binding) {
        if (bound.first == var)
            BUG("Variable %1% already bound", var->toString());
        const IR::Type* type = bound.second;
        const IR::Node* newType = type->apply(visitor);
        if (newType == nullptr)
            return false;

        binding.emplace(bound.first, newType->to<IR::Type>());
    }

    success = setBinding(var, substitution);
    if (!success)
        BUG("Failed to insert binding");
    return true;
}

bool TypeVariableSubstitution::setBindings(const IR::Node* root,
                                           const IR::TypeParameters* params,
                                           const IR::Vector<IR::Type>* args) {
    if (params == nullptr || args == nullptr)
        BUG("Nullptr argument to setBindings");

    if (params->parameters.size() != args->size()) {
        ::error("%1% has %2% type parameters, invoked with %3% %4%",
                root, params->parameters.size(), args->size(), args);
        return false;
    }

    auto it = args->begin();
    for (auto tp : *params->getEnumerator()) {
        auto t = *it;
        ++it;

        bool success = setBinding(tp, t);
        if (!success) {
            ::error("Cannot bind %1% to %2%", tp, t);
            return false;
        }
    }

    return true;
}

}  // namespace IR

