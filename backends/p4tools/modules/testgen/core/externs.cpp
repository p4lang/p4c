#include "backends/p4tools/modules/testgen/core/externs.h"

#include <list>
#include <optional>
#include <tuple>
#include <vector>

#include "backends/p4tools/common/lib/util.h"
#include "ir/ir.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/null.h"

#include "backends/p4tools/modules/testgen/core/small_step/small_step.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools::P4Testgen {

bool ExternMethodImpls::exec(const IR::MethodCallExpression *call, const IR::Expression *receiver,
                             IR::ID &name, const IR::Vector<IR::Argument> *args,
                             const ExecutionState &state,
                             SmallStepEvaluator::Result &result) const {
    // We have to check the extern type here. We may receive a specialized canonical type, which we
    // need to unpack.
    const IR::Type_Extern *externType = nullptr;
    if (const auto *type = receiver->type->to<IR::Type_Extern>()) {
        externType = type;
    } else if (const auto *specType = receiver->type->to<IR::Type_SpecializedCanonical>()) {
        CHECK_NULL(specType->substituted);
        externType = specType->substituted->checkedTo<IR::Type_Extern>();
    }

    BUG_CHECK(externType, "Not an extern: %1%", receiver);

    cstring qualifiedMethodName = externType->name + "." + name;
    if (impls.count(qualifiedMethodName) == 0) {
        return false;
    }

    const auto &submap = impls.at(qualifiedMethodName);
    if (submap.count(args->size()) == 0) {
        return false;
    }

    // Find matching methods: if any arguments are named, then the parameter name must match.
    std::optional<MethodImpl> matchingImpl;
    for (const auto &pair : submap.at(args->size())) {
        const auto &paramNames = pair.first;
        const auto &methodImpl = pair.second;

        if (matches(paramNames, args)) {
            BUG_CHECK(!matchingImpl, "Ambiguous extern method call: %1%", name);
            matchingImpl = methodImpl;
        }
    }

    if (!matchingImpl) {
        return false;
    }
    (*matchingImpl)(call, receiver, name, args, state, result);
    return true;
}

bool ExternMethodImpls::matches(const std::vector<cstring> &paramNames,
                                const IR::Vector<IR::Argument> *args) {
    CHECK_NULL(args);

    // Number of parameters should match the number of arguments.
    if (paramNames.size() != args->size()) {
        return false;
    }
    // Any named argument should match the name of the corresponding parameter.
    for (size_t idx = 0; idx < paramNames.size(); idx++) {
        const auto &paramName = paramNames.at(idx);
        const auto &arg = args->at(idx);

        if (arg->name.name == nullptr) {
            continue;
        }
        if (paramName != arg->name.name) {
            return false;
        }
    }

    return true;
}

ExternMethodImpls::ExternMethodImpls(const ImplList &inputImplList) {
    for (const auto &implSpec : inputImplList) {
        cstring name;
        std::vector<cstring> paramNames;
        MethodImpl impl;
        std::tie(name, paramNames, impl) = implSpec;

        auto &tmpImplList = impls[name][paramNames.size()];

        // Make sure that we have at most one implementation for each set of parameter names.
        // This is a quadratic-time algorithm, but should be fine, since we expect the number of
        // overloads to be small in practice.
        for (auto &pair : tmpImplList) {
            BUG_CHECK(pair.first != paramNames, "Multiple implementations of %1%(%2%)", name,
                      Utils::containerToString(paramNames));
        }

        tmpImplList.emplace_back(paramNames, impl);
    }
}

}  // namespace P4Tools::P4Testgen
