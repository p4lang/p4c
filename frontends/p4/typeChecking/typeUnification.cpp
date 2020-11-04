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
bool TypeUnification::unifyCall(const EqualityConstraint* constraint) {
    // These are canonical types.
    auto dest = constraint->left->to<IR::Type_MethodBase>();
    auto src = constraint->right->to<IR::Type_MethodCall>();
    CHECK_NULL(dest); CHECK_NULL(src);
    LOG3("Unifying function " << dest << " with caller " << src);

    for (auto tv : dest->typeParameters->parameters)
        constraints->addUnifiableTypeVariable(tv);
    if (dest->returnType == nullptr)
        constraints->addEqualityConstraint(IR::Type_Void::get(), src->returnType, constraint);
    else
        constraints->addEqualityConstraint(dest->returnType, src->returnType, constraint);
    constraints->addUnifiableTypeVariable(src->returnType);  // always a type variable

    for (auto tv : dest->typeParameters->parameters)
        constraints->addUnifiableTypeVariable(tv);

    if (src->typeArguments->size() != 0) {
        if (dest->typeParameters->size() != src->typeArguments->size()) {
            constraint->reportError(
                "%1% type parameters expected, but %2% type arguments supplied",
                dest->typeParameters->size(), src->typeArguments->size());
            return false;
        }

        size_t i = 0;
        for (auto tv : dest->typeParameters->parameters) {
            auto type = src->typeArguments->at(i++);
            // variable type represents type of formal method argument
            // written beetween angle brackets, and tv should be replaced
            // with type of an actual argument
            constraints->addEqualityConstraint(type /*dst */, tv /* src */, constraint);
        }
    }

    if (dest->parameters->size() < src->arguments->size()) {
        constraint->reportError(
            "%1% arguments received when %2% expected",
            src->arguments->size(), dest->parameters->size());
        return false;
    }

    auto paramIt = dest->parameters->begin();
    // keep track of parameters that have not been matched yet
    std::map<cstring, const IR::Parameter*> left;
    for (auto p : dest->parameters->parameters)
        left.emplace(p->name, p);

    for (auto arg : *src->arguments) {
        cstring argName = arg->argument->name.name;
        bool named = !argName.isNullOrEmpty();
        const IR::Parameter* param;

        if (named) {
            param = dest->parameters->getParameter(argName);
            if (param == nullptr) {
                TypeInference::typeError(
                    "No parameter named %1%", arg->argument->name);
                return false;
            }
        } else {
            if (paramIt == dest->parameters->end()) {
                TypeInference::typeError("Too many arguments for call");
                return false;
            }
            param = *paramIt;
        }

        auto leftIt = left.find(param->name.name);
        // This shold have been checked by the CheckNamedArgs pass.
        BUG_CHECK(leftIt != left.end(), "%1%: Duplicate argument name?", param->name);
        left.erase(leftIt);

        if (arg->type->is<IR::Type_Dontcare>() && param->direction != IR::Direction::Out) {
            TypeInference::typeError(
                "%1%: don't care argument only allowed for out parameters", arg->srcInfo);
            return false;
        }
        if ((param->direction == IR::Direction::Out || param->direction == IR::Direction::InOut) &&
            (!arg->leftValue)) {
            TypeInference::typeError("%1%: Read-only value used for out/inout parameter %2%",
                                     arg->srcInfo, param);
            return false;
        } else if (param->direction == IR::Direction::None && !arg->compileTimeConstant) {
            constraint->reportError("%1%: argument used for directionless parameter %2% "
                                    "must be a compile-time constant", arg->argument, param);
            return false;
        }

        if (param->direction != IR::Direction::None && param->type->is<IR::Type_Extern>()) {
            if (optarg) continue;
            constraint->reportError("%1%: extern values cannot be passed in/out/inout", param);
            return false;
        }

        constraints->addEqualityConstraint(param->type, arg->type, constraint);
        if (!named)
            ++paramIt;
    }

    // Check remaining parameters: they must be all optional or have default values
    for (auto p : left) {
        bool opt = p.second->isOptional() || p.second->defaultValue != nullptr;
        if (opt) continue;
        constraint->reportError("No argument for parameter %1%", p.second);
        return false;
    }

    return true;
}

// skipReturnValues is needed because the return type of a package
// is the package itself, so checking it gets into an infinite loop.
bool TypeUnification::unifyFunctions(const EqualityConstraint* constraint,
                                     bool skipReturnValues) {
    auto dest = constraint->left->to<IR::Type_MethodBase>();
    auto src = constraint->right->to<IR::Type_MethodBase>();
    CHECK_NULL(dest); CHECK_NULL(src);
    LOG3("Unifying functions " << dest << " with " << src);

    for (auto tv : dest->typeParameters->parameters)
        constraints->addUnifiableTypeVariable(tv);
    for (auto tv : src->typeParameters->parameters)
        constraints->addUnifiableTypeVariable(tv);

    if ((src->returnType == nullptr) != (dest->returnType == nullptr)) {
        constraint->reportError("Cannot unify functions with different return types"
                                " %1% and %2%", dest, src);
        return false;
    }
    if (!skipReturnValues && src->returnType != nullptr)
        constraints->addEqualityConstraint(dest->returnType, src->returnType, constraint);

    auto sit = src->parameters->parameters.begin();
    for (auto dit : *dest->parameters->getEnumerator()) {
        if (sit == src->parameters->parameters.end()) {
            if (dit->isOptional())
                continue;
            if (dit->defaultValue != nullptr)
                continue;
            constraint->reportError(
                "Cannot unify functions with different number of arguments: "
                "%1% to %2%", src, dest);
            return false; }
        if ((*sit)->direction != dit->direction) {
            constraint->reportError("Cannot unify parameter %1% with %2% "
                                    "because they have different directions",
                                    *sit, dit);
            return false;
        }
        constraints->addEqualityConstraint(dit->type, (*sit)->type, constraint);
        ++sit;
    }
    while (sit != src->parameters->parameters.end()) {
        if ((*sit)->isOptional()) {
            ++sit;
            continue; }
        if ((*sit)->defaultValue != nullptr) {
            ++sit;
            continue; }
        constraint->reportError(
            "Cannot unify functions with different number of arguments: "
            "%1% to %2%", src, dest);
        return false; }
    return true;
}

bool TypeUnification::unifyBlocks(const EqualityConstraint* constraint) {
    // These are canonical types.
    auto dest = constraint->left->to<IR::Type_ArchBlock>();
    auto src = constraint->right->to<IR::Type_ArchBlock>();
    CHECK_NULL(dest); CHECK_NULL(src);
    LOG3("Unifying blocks " << dest << " with " << src);
    if (typeid(*dest) != typeid(*src)) {
        constraint->reportError();
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
            constraint->reportError();
            return false;
        }
        auto destConstructor = dest->to<IR::Type_Package>()->getConstructorMethodType();
        auto srcConstructor = src->to<IR::Type_Package>()->getConstructorMethodType();
        return unifyFunctions(new EqualityConstraint(
            destConstructor, srcConstructor, constraint), true);
    } else if (dest->is<IR::IApply>()) {
        // parsers, controls
        auto srcapply = src->to<IR::IApply>()->getApplyMethodType();
        auto destapply = dest->to<IR::IApply>()->getApplyMethodType();
        return unifyFunctions(
            new EqualityConstraint(destapply, srcapply, constraint));
    }
    constraint->reportError();
    return false;
}

bool TypeUnification::unify(const EqualityConstraint* constraint) {
    auto dest = constraint->left;
    auto src = constraint->right;
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
            constraint->reportError();
            return false;
        }
        auto bc = new EqualityConstraint(dest, src, constraint);
        return unifyBlocks(bc);
    } else if (dest->is<IR::Type_MethodBase>()) {
        auto destt = dest->to<IR::Type_MethodBase>();
        auto srct = src->to<IR::Type_MethodCall>();
        if (srct != nullptr)
            return unifyCall(constraint);
        auto srcf = src->to<IR::Type_MethodBase>();
        if (srcf != nullptr)
            return unifyFunctions(new EqualityConstraint(destt, srcf, constraint));
        constraint->reportError("Cannot unify non-function type %1% to function type %2%",
                                src, dest);
        return false;
    } else if (auto td = dest->to<IR::Type_BaseList>()) {
        if (!src->is<IR::Type_BaseList>()) {
            constraint->reportError();
            return false;
        }
        auto ts = src->to<IR::Type_BaseList>();
        if (td->components.size() != ts->components.size()) {
            constraint->reportError("Tuples with different sizes %1% vs %2%",
                                    td->components.size(), ts->components.size());
            return false;
        }

        for (size_t i=0; i < td->components.size(); i++) {
            auto si = ts->components.at(i);
            auto di = td->components.at(i);
            constraints->addEqualityConstraint(di, si, constraint);
        }
        return true;
    } else if (dest->is<IR::Type_Struct>() || dest->is<IR::Type_Header>()) {
        auto strct = dest->to<IR::Type_StructLike>();
        if (auto tpl = src->to<IR::Type_List>()) {
            if (strct->fields.size() != tpl->components.size()) {
                constraint->reportError("Number of fields %1% in initializer different "
                                        "than number of fields in structure %2%: %3% to %4%",
                                         tpl->components.size(), strct->fields.size(), tpl, strct);
                return false;
            }

            int index = 0;
            for (const IR::StructField* f : strct->fields) {
                const IR::Type* tplField = tpl->components.at(index);
                const IR::Type* destt = f->type;
                constraints->addEqualityConstraint(destt, tplField, constraint);
                index++;
            }
            return true;
        } else if (auto st = src->to<IR::Type_StructLike>()) {
            if (strct->name != st->name &&
                !st->is<IR::Type_UnknownStruct>() &&
                !strct->is<IR::Type_UnknownStruct>()) {
                constraint->reportError("Cannot unify %1% with %2%",
                                         st->name, strct->name);
                return false;
            }
            // There is another case, in which each field of the source is unifiable with the
            // corresponding field of the destination, e.g., a struct containing tuples.
            if (strct->fields.size() != st->fields.size()) {
                constraint->reportError("Number of fields %1% in initializer different "
                                        "than number of fields in structure %2%: %3% to %4%",
                                        st->fields.size(), strct->fields.size(), st, strct);
                return false;
            }

            for (const IR::StructField* f : strct->fields) {
                auto stField = st->getField(f->name);
                if (stField == nullptr) {
                    constraint->reportError("No initializer for field %1%", f);
                    return false;
                }
                constraints->addEqualityConstraint(f->type, stField->type, constraint);
            }
            return true;
        }

        constraint->reportError();
        return false;
    } else if (dest->is<IR::Type_Base>()) {
        if (dest->is<IR::Type_Bits>() && src->is<IR::Type_InfInt>()) {
            constraints->addUnifiableTypeVariable(src->to<IR::Type_InfInt>());
            constraints->addEqualityConstraint(dest, src, constraint);
            return true;
        }
        if (auto senum = src->to<IR::Type_SerEnum>()) {
            if (dest->is<IR::Type_Bits>())
                // unify with enum's underlying type
                return unify(new EqualityConstraint(senum->type, dest, constraint));
        }
        if (!src->is<IR::Type_Base>()) {
            constraint->reportError();
            return false;
        }

        bool success = (*src) == (*dest);
        if (!success) {
            constraint->reportError();
            return false;
        }

        return true;
    } else if (dest->is<IR::Type_Declaration>() && src->is<IR::Type_Declaration>()) {
        bool canUnify = typeid(dest) == typeid(src) &&
            dest->to<IR::Type_Declaration>()->name == src->to<IR::Type_Declaration>()->name;
        if (!canUnify)
            constraint->reportError();
        return canUnify;
    } else if (dest->is<IR::Type_Stack>() && src->is<IR::Type_Stack>()) {
        auto dstack = dest->to<IR::Type_Stack>();
        auto sstack = src->to<IR::Type_Stack>();
        if (dstack->getSize() != sstack->getSize()) {
            constraint->reportError(
                "cannot unify stacks with different sizes %1% and %2%",
                dstack, sstack);
            return false;
        }
        constraints->addEqualityConstraint(dstack->elementType, sstack->elementType, constraint);
        return true;
    }

    constraint->reportError();
    return false;
}

}  // namespace P4
