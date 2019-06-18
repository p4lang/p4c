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

#include "typeUnification.h"
#include "typeConstraints.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"

namespace P4 {

/// Unifies a call with a prototype.
bool TypeUnification::unifyCall(const IR::Node* errorPosition,
                                const IR::Type_MethodBase* dest,
                                const IR::Type_MethodCall* src,
                                bool reportErrors) {
    // These are canonical types.
    CHECK_NULL(dest); CHECK_NULL(src);
    LOG3("Unifying function " << dest << " with caller " << src);

    for (auto tv : dest->typeParameters->parameters)
        constraints->addUnifiableTypeVariable(tv);
    if (dest->returnType == nullptr)
        constraints->addEqualityConstraint(IR::Type_Void::get(), src->returnType);
    else
        constraints->addEqualityConstraint(dest->returnType, src->returnType);
    constraints->addUnifiableTypeVariable(src->returnType);  // always a type variable

    for (auto tv : dest->typeParameters->parameters)
        constraints->addUnifiableTypeVariable(tv);

    if (src->typeArguments->size() != 0) {
        if (dest->typeParameters->size() != src->typeArguments->size()) {
            TypeInference::typeError(
                "%1% has %2% type parameters, but is invoked with %3% type arguments",
                errorPosition, (dest->typeParameters ? dest->typeParameters->size() : 0),
                src->typeArguments->size());
            return false;
        }

        size_t i = 0;
        for (auto tv : dest->typeParameters->parameters) {
            auto type = src->typeArguments->at(i++);
            constraints->addEqualityConstraint(tv, type);
        }
    }

    if (dest->parameters->size() < src->arguments->size()) {
        if (reportErrors)
            TypeInference::typeError(
                "%1%: Passing %2% arguments when %3% expected",
                errorPosition, src->arguments->size(), dest->parameters->size());
        return false;
    }

    auto paramIt = dest->parameters->begin();
    // keep track of parameters that have not been matched yet
    std::map<cstring, const IR::Parameter*> left;
    for (auto p : dest->parameters->parameters)
        left.emplace(p->name, p);

    for (auto arg : *src->arguments) {
        cstring argName = arg->name.name;
        bool named = !argName.isNullOrEmpty();
        const IR::Parameter* param;

        if (named) {
            param = dest->parameters->getParameter(argName);
            if (param == nullptr) {
                if (reportErrors)
                    TypeInference::typeError(
                        "%1%: No parameter named %2%", errorPosition, arg->name);
                return false;
            }
        } else {
            if (paramIt == dest->parameters->end()) {
                if (reportErrors)
                    TypeInference::typeError("%1%: Too many arguments for call", errorPosition);
                return false;
            }
            param = *paramIt;
        }

        auto leftIt = left.find(param->name.name);
        // This shold have been checked by the CheckNamedArgs pass.
        BUG_CHECK(leftIt != left.end(), "%1%: Duplicate argument name?", param->name);
        left.erase(leftIt);

        if (arg->type->is<IR::Type_Dontcare>() && param->direction != IR::Direction::Out) {
            if (reportErrors)
                TypeInference::typeError(
                    "%1%: don't care argument only allowed for out parameters", arg->srcInfo);
            return false;
        }
        if ((param->direction == IR::Direction::Out || param->direction == IR::Direction::InOut) &&
            (!arg->leftValue)) {
            if (reportErrors)
                TypeInference::typeError("%1%: Read-only value used for out/inout parameter %2%",
                                         arg->srcInfo, param);
            return false;
        } else if (param->direction == IR::Direction::None && !arg->compileTimeConstant) {
            if (reportErrors)
                TypeInference::typeError("%1%: not a compile-time constant when binding to %2%",
                                         arg->srcInfo, param);
            return false;
        }

        if (param->direction != IR::Direction::None && param->type->is<IR::Type_Extern>()) {
            if (optarg) continue;
            TypeInference::typeError("%1%: extern values cannot be passed in/out/inout", param);
            return false;
        }

        constraints->addEqualityConstraint(param->type, arg->type);
        if (!named)
            ++paramIt;
    }

    // Check remaining parameters: they must be all optional or have default values
    for (auto p : left) {
        bool opt = p.second->isOptional() || p.second->defaultValue != nullptr;
        if (opt) continue;
        if (reportErrors)
            TypeInference::typeError("%1%: No argument for parameter %2%", errorPosition, p.second);
        return false;
    }

    return true;
}

// skipReturnValues is needed because the return type of a package
// is the package itself, so checking it gets into an infinite loop.
bool TypeUnification::unifyFunctions(const IR::Node* errorPosition,
                                     const IR::Type_MethodBase* dest,
                                     const IR::Type_MethodBase* src,
                                     bool reportErrors,
                                     bool skipReturnValues) {
    CHECK_NULL(dest); CHECK_NULL(src);
    LOG3("Unifying functions " << dest << " with " << src);

    for (auto tv : dest->typeParameters->parameters)
        constraints->addUnifiableTypeVariable(tv);
    for (auto tv : src->typeParameters->parameters)
        constraints->addUnifiableTypeVariable(tv);

    if ((src->returnType == nullptr) != (dest->returnType == nullptr)) {
        if (reportErrors)
            TypeInference::typeError("%1%: Cannot unify functions with different return types"
                                     " %2% and %3%", errorPosition, dest, src);
        return false;
    }
    if (!skipReturnValues && src->returnType != nullptr)
        constraints->addEqualityConstraint(dest->returnType, src->returnType);

    auto sit = src->parameters->parameters.begin();
    for (auto dit : *dest->parameters->getEnumerator()) {
        if (sit == src->parameters->parameters.end()) {
            if (dit->isOptional())
                continue;
            if (dit->defaultValue != nullptr)
                continue;
            if (reportErrors)
                TypeInference::typeError(
                    "%1%: Cannot unify functions with different number of arguments: "
                    "%2% to %3%", errorPosition, src, dest);
            return false; }
        if ((*sit)->direction != dit->direction) {
            if (reportErrors)
                TypeInference::typeError("%1%: Cannot unify parameter %2% with %3% "
                                         "because they have different directions",
                                         errorPosition, *sit, dit);
            return false;
        }
        constraints->addEqualityConstraint(dit->type, (*sit)->type);
        ++sit;
    }
    while (sit != src->parameters->parameters.end()) {
        if ((*sit)->isOptional()) {
            ++sit;
            continue; }
        if ((*sit)->defaultValue != nullptr) {
            ++sit;
            continue; }
        if (reportErrors)
            TypeInference::typeError(
                "%1%: Cannot unify functions with different number of arguments: "
                "%2% to %3%", errorPosition, src, dest);
        return false; }
    return true;
}

bool TypeUnification::unifyBlocks(const IR::Node* errorPosition,
                                  const IR::Type_ArchBlock* dest,
                                  const IR::Type_ArchBlock* src,
                                  bool reportErrors) {
    // These are canonical types.
    CHECK_NULL(dest); CHECK_NULL(src);
    LOG3("Unifying blocks " << dest << " with " << src);
    if (typeid(*dest) != typeid(*src)) {
        if (reportErrors)
            TypeInference::typeError("%1%: Cannot unify %2% to %3%",
                                     errorPosition, src->toString(), dest->toString());
        return false;
    }
    for (auto tv : dest->typeParameters->parameters)
        constraints->addUnifiableTypeVariable(tv);
    for (auto tv : src->typeParameters->parameters)
        constraints->addUnifiableTypeVariable(tv);
    if (dest->is<IR::Type_Package>()) {
        // Two packages unify if and only if they have the same name
        // and if their corresponding parameters unify
        auto destPackage = dest->to<IR::Type_Package>();
        auto srcPackage = src->to<IR::Type_Package>();
        if (destPackage->name != srcPackage->name) {
            if (reportErrors)
                TypeInference::typeError("%1%: Cannot unify %2% to %3%",
                                         errorPosition, src->toString(), dest->toString());
            return false;
        }
        auto destConstructor = dest->to<IR::Type_Package>()->getConstructorMethodType();
        auto srcConstructor = src->to<IR::Type_Package>()->getConstructorMethodType();
        bool success = unifyFunctions(
            errorPosition, destConstructor, srcConstructor, reportErrors, true);
        return success;
    } else if (dest->is<IR::IApply>()) {
        // parsers, controls
        auto srcapply = src->to<IR::IApply>()->getApplyMethodType();
        auto destapply = dest->to<IR::IApply>()->getApplyMethodType();
        bool success = unifyFunctions(errorPosition, destapply, srcapply, reportErrors);
        return success;
    }
    TypeInference::typeError("%1%: Cannot unify %2% to %3%",
                             errorPosition, src->toString(), dest->toString());
    return false;
}

bool TypeUnification::unify(const IR::Node* errorPosition,
                            const IR::Type* dest,
                            const IR::Type* src,
                            bool reportErrors) {
    // These are canonical types.
    CHECK_NULL(dest); CHECK_NULL(src);
    LOG3("Unifying " << dest << " with " << src);

    if (src->is<IR::ITypeVar>())
        src = src->apply(constraints->replaceVariables)->to<IR::Type>();
    if (dest->is<IR::ITypeVar>())
        dest = dest->apply(constraints->replaceVariables)->to<IR::Type>();

    if (TypeMap::equivalent(dest, src))
        return true;

    if (dest->is<IR::Type_SpecializedCanonical>())
        dest = dest->to<IR::Type_SpecializedCanonical>()->substituted;
    if (src->is<IR::Type_SpecializedCanonical>())
        src = src->to<IR::Type_SpecializedCanonical>()->substituted;

    if (src->is<IR::Type_Dontcare>() || dest->is<IR::Type_Dontcare>())
        return true;

    if (dest->is<IR::Type_ArchBlock>()) {
        if (!src->is<IR::Type_ArchBlock>()) {
            if (reportErrors)
                TypeInference::typeError("%1%: Cannot unify %2% to %3%",
                                         errorPosition, src->toString(), dest->toString());
            return false;
        }
        return unifyBlocks(errorPosition, dest->to<IR::Type_ArchBlock>(),
                           src->to<IR::Type_ArchBlock>(), reportErrors);
    } else if (dest->is<IR::Type_MethodBase>()) {
        auto destt = dest->to<IR::Type_MethodBase>();
        auto srct = src->to<IR::Type_MethodCall>();
        if (srct != nullptr)
            return unifyCall(errorPosition, destt, srct, reportErrors);
        auto srcf = src->to<IR::Type_MethodBase>();
        if (srcf != nullptr)
            return unifyFunctions(errorPosition, destt, srcf, reportErrors);

        if (reportErrors)
            TypeInference::typeError("%1%: Cannot unify non-function type %2% to function type %3%",
                                     errorPosition, src->toString(), dest->toString());
        return false;
    } else if (dest->is<IR::Type_Tuple>()) {
        if (src->is<IR::Type_Struct>() || src->is<IR::Type_Header>()) {
            // swap and try again: handled below
            return unify(errorPosition, src, dest, reportErrors);
        }
        if (!src->is<IR::Type_Tuple>()) {
            if (reportErrors)
                TypeInference::typeError("%1%: Cannot unify tuple type %2% with non tuple-type %3%",
                                         errorPosition, dest->toString(), src->toString());
            return false;
        }
        auto td = dest->to<IR::Type_Tuple>();
        auto ts = src->to<IR::Type_Tuple>();
        if (td->components.size() != ts->components.size()) {
            TypeInference::typeError("%1%: Cannot match tuples with different sizes %2% vs %3%",
                                     errorPosition, td->components.size(), ts->components.size());
            return false;
        }

        for (size_t i=0; i < td->components.size(); i++) {
            auto si = ts->components.at(i);
            auto di = td->components.at(i);
            bool success = unify(errorPosition, di, si, reportErrors);
            if (!success)
                return false;
        }
        return true;
    } else if (dest->is<IR::Type_Struct>() || dest->is<IR::Type_Header>()) {
        auto strct = dest->to<IR::Type_StructLike>();
        if (auto tpl = src->to<IR::Type_Tuple>()) {
            if (strct->fields.size() != tpl->components.size()) {
                if (reportErrors)
                    TypeInference::typeError("%1%: Number of fields %2% in initializer different "
                                             "than number of fields in structure %3%: %4% to %5%",
                                             errorPosition, tpl->components.size(),
                                             strct->fields.size(), tpl, strct);
                return false;
            }

            int index = 0;
            for (const IR::StructField* f : strct->fields) {
                const IR::Type* tplField = tpl->components.at(index);
                const IR::Type* destt = f->type;

                bool success = unify(errorPosition, destt, tplField, reportErrors);
                if (!success)
                    return false;
                index++;
            }
            return true;
        } else if (auto st = src->to<IR::Type_StructLike>()) {
            // There is another case, in which each field of the source is unifiable with the
            // corresponding field of the destination, e.g., a struct containing tuples.
            if (strct->fields.size() != st->fields.size()) {
                if (reportErrors)
                    TypeInference::typeError("%1%: Number of fields %2% in initializer different "
                                             "than number of fields in structure %3%: %4% to %5%",
                                             errorPosition, st->fields.size(),
                                             strct->fields.size(), st, strct);
                return false;
            }

            for (const IR::StructField* f : strct->fields) {
                auto stField = st->getField(f->name);
                if (stField == nullptr) {
                    TypeInference::typeError("%1%: No initializer for field %2%", errorPosition, f);
                    return false;
                }
                bool success = unify(errorPosition, f->type, stField->type, reportErrors);
                if (!success)
                    return false;
            }
            return true;
        }

        if (reportErrors)
            TypeInference::typeError("%1%: Cannot unify %2% to %3%",
                                     errorPosition, src->toString(), dest->toString());
        return false;
    } else if (dest->is<IR::Type_Base>()) {
        if (dest->is<IR::Type_Bits>() && src->is<IR::Type_InfInt>()) {
            constraints->addUnifiableTypeVariable(src->to<IR::Type_InfInt>());
            constraints->addEqualityConstraint(dest, src);
            return true;
        }
        if (!src->is<IR::Type_Base>()) {
            if (reportErrors)
                TypeInference::typeError("%1%: Cannot unify %2% to %3%",
                                         errorPosition, src->toString(), dest->toString());
            return false;
        }

        bool success = (*src) == (*dest);
        if (!success) {
            if (reportErrors)
                TypeInference::typeError("%1%: Cannot unify %2% to %3%",
                                         errorPosition, src->toString(), dest->toString());
            return false;
        }

        return true;
    } else if (dest->is<IR::Type_Declaration>() && src->is<IR::Type_Declaration>()) {
        bool canUnify = typeid(dest) == typeid(src) &&
            dest->to<IR::Type_Declaration>()->name == src->to<IR::Type_Declaration>()->name;
        if (!canUnify && reportErrors)
            TypeInference::typeError("%1%: Cannot unify %2% to %3%",
                                     errorPosition, src->toString(), dest->toString());
        return canUnify;
    } else if (dest->is<IR::Type_Stack>() && src->is<IR::Type_Stack>()) {
        auto dstack = dest->to<IR::Type_Stack>();
        auto sstack = src->to<IR::Type_Stack>();
        if (dstack->getSize() != sstack->getSize()) {
            if (reportErrors)
                TypeInference::typeError(
                    "%1%: cannot unify stacks with different sized %2% and %3%",
                    errorPosition, dstack, sstack);
            return false;
        }
        constraints->addEqualityConstraint(dstack->elementType, sstack->elementType);
        return true;
    }

    if (reportErrors)
        TypeInference::typeError("%1%: Cannot unify %2% to %3%", errorPosition, src, dest);
    return false;
}

}  // namespace P4
