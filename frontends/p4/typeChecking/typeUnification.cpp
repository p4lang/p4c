#include "typeUnification.h"
#include "typeConstraints.h"

namespace P4 {
bool TypeUnification::unifyFunctions(const IR::Node* errorPosition,
                                     const IR::Type_MethodBase* dest,
                                     const IR::Type_MethodCall* src,
                                     bool reportErrors) {
    CHECK_NULL(dest); CHECK_NULL(src);
    LOG1("Unifying function " << dest << " with caller " << src);

    if (dest->returnType == nullptr)
        constraints->addEqualityConstraint(IR::Type_Void::get(), src->returnType);
    else
        constraints->addEqualityConstraint(dest->returnType, src->returnType);

    for (auto tv : *dest->typeParameters->parameters)
        constraints->addUnifiableTypeVariable(tv);
    constraints->addUnifiableTypeVariable(src->returnType);  // always a type variable

    if (src->typeArguments->size() != 0) {
        if (dest->typeParameters->size() != src->typeArguments->size()) {
            ::error("%1% has %2% type parameters, but is invoked with %3% type arguments",
                    errorPosition, (dest->typeParameters ? dest->typeParameters->size() : 0),
                    src->typeArguments->size());
            return false;
        }

        size_t i = 0;
        for (auto tv : *dest->typeParameters->parameters) {
            auto type = src->typeArguments->at(i++);
            constraints->addEqualityConstraint(tv, type);
        }
    }

    if (dest->parameters->size() != src->arguments->size()) {
        if (reportErrors)
            ::error("%1%: Passing %2% arguments when %3% expected",
                    errorPosition, src->arguments->size(), dest->parameters->size());
        return false;
    }

    auto sit = src->arguments->begin();
    for (auto dit : *dest->parameters->getEnumerator()) {
        auto arg = *sit;
        if ((dit->direction == IR::Direction::Out || dit->direction == IR::Direction::InOut) &&
            (!arg->leftValue)) {
            if (reportErrors)
                ::error("%1%: Read-only value used for out/inout parameter %2%", arg->srcInfo, dit);
            return false;
        }
        constraints->addEqualityConstraint(dit->type, arg->type);
        ++sit;
    }

    return true;
}

bool TypeUnification::unifyFunctions(const IR::Node* errorPosition,
                                     const IR::Type_MethodBase* dest,
                                     const IR::Type_MethodBase* src,
                                     bool reportErrors) {
    CHECK_NULL(dest); CHECK_NULL(src);
    LOG1("Unifying functions " << dest << " to " << src);

    for (auto tv : *dest->typeParameters->parameters)
        constraints->addUnifiableTypeVariable(tv);
    for (auto tv : *src->typeParameters->parameters)
        constraints->addUnifiableTypeVariable(tv);

    if ((src->returnType == nullptr) != (dest->returnType == nullptr)) {
        if (reportErrors)
            ::error("%1%: Cannot unify functions with different return types"
                    " %2% and %3%", errorPosition,
                    dest, src);
        return false;
    }
    if (src->returnType != nullptr)
        constraints->addEqualityConstraint(dest->returnType, src->returnType);
    if (dest->parameters->size() != src->parameters->size()) {
        if (reportErrors)
            ::error("Cannot unify functions with different number of arguments: %1% to %2%",
                    src, dest);
        return false;
    }

    auto sit = src->parameters->parameters->begin();
    for (auto dit : *dest->parameters->getEnumerator()) {
        if ((*sit)->direction != dit->direction) {
            if (reportErrors)
                ::error("Cannot unify parameter %1% with %2% "
                        "because they have different directions", *sit, dit);
            return false;
        }
        constraints->addEqualityConstraint(dit->type, (*sit)->type);
        ++sit;
    }

    return true;
}

bool TypeUnification::unifyBlocks(const IR::Node* errorPosition,
                                  const IR::Type_ArchBlock* dest,
                                  const IR::Type_ArchBlock* src,
                                  bool reportErrors) {
    CHECK_NULL(dest); CHECK_NULL(src);
    LOG1("Unifying blocks " << dest << " to " << src);
    if (typeid(*dest) != typeid(*src)) {
        if (reportErrors)
            ::error("%1%: Cannot unify %2% to %3%",
                    errorPosition, src->toString(), dest->toString());
        return false;
    }
    if (dest->is<IR::IApply>()) {
        auto srcapply = src->to<IR::IApply>()->getApplyMethodType();
        auto destapply = dest->to<IR::IApply>()->getApplyMethodType();
        bool success = unifyFunctions(errorPosition, destapply, srcapply, reportErrors);
        return success;
    }
    ::error("%1%: Cannot unify %2% to %3%",
            errorPosition, src->toString(), dest->toString());
    return false;
}

bool TypeUnification::unify(const IR::Node* errorPosition,
                            const IR::Type* dest,
                            const IR::Type* src,
                            bool reportErrors) {
    CHECK_NULL(dest); CHECK_NULL(src);
    LOG1("Unifying " << dest->toString() << " to " << src->toString());

    if (dest == src)
        return true;

    if (dest->is<IR::Type_SpecializedCanonical>())
        dest = dest->to<IR::Type_SpecializedCanonical>()->substituted;
    if (src->is<IR::Type_SpecializedCanonical>())
        src = src->to<IR::Type_SpecializedCanonical>()->substituted;

    if (dest == IR::Type_Dontcare::get())
        return true;

    if (dest->is<IR::Type_ArchBlock>()) {
        if (!src->is<IR::Type_ArchBlock>()) {
            if (reportErrors)
                ::error("%1%: Cannot unify %2% to %3%",
                        errorPosition, dest->toString(), src->toString());
            return false;
        }
        return unifyBlocks(errorPosition, dest->to<IR::Type_ArchBlock>(),
                           src->to<IR::Type_ArchBlock>(), reportErrors);
    } else if (dest->is<IR::Type_MethodBase>()) {
        auto destt = dest->to<IR::Type_MethodBase>();
        auto srct = src->to<IR::Type_MethodCall>();
        if (srct != nullptr)
            return unifyFunctions(errorPosition, destt, srct, reportErrors);
        auto srcf = src->to<IR::Type_MethodBase>();
        if (srcf != nullptr)
            return unifyFunctions(errorPosition, destt, srcf, reportErrors);

        if (reportErrors)
            ::error("%1%: Cannot unify non-function type %2% to function type %3%",
                    errorPosition, src->toString(), dest->toString());
        return false;
    } else if (dest->is<IR::Type_Tuple>()) {
        if (src->is<IR::Type_Struct>()) {
            // swap and try again: handled below
            return unify(errorPosition, src, dest, reportErrors);
        }
        if (!src->is<IR::Type_Tuple>()) {
            if (reportErrors)
                ::error("%1%: Cannot unify tuple type %2% with non tuple-type %3%",
                        errorPosition, dest->toString(), src->toString());
            return false;
        }
        auto td = dest->to<IR::Type_Tuple>();
        auto ts = src->to<IR::Type_Tuple>();
        if (td->components->size() != ts->components->size()) {
            ::error("%1%: Cannot match tuples with different sizes %2% vs %3%",
                    errorPosition, td->components->size(), ts->components->size());
            return false;
        }

        for (size_t i=0; i < td->components->size(); i++) {
            auto si = ts->components->at(i);
            auto di = td->components->at(i);
            bool success = unify(errorPosition, di, si, reportErrors);
            if (!success)
                return false;
            return true;
        }
    } else if (dest->is<IR::Type_Struct>()) {
        const IR::Type_Struct *strct = dest->to<IR::Type_Struct>();
        if (src->is<IR::Type_Tuple>()) {
            const IR::Type_Tuple* tpl = src->to<IR::Type_Tuple>();
            if (strct->fields->size() != tpl->components->size()) {
                if (reportErrors)
                    ::error("%1%: Number of fields %2% in initializer different "
                            "than number of fields in structure %3%: %4% to %5%",
                            errorPosition, tpl->components->size(),
                            strct->fields->size(), tpl, strct);
                return false;
            }

            int index = 0;
            for (const IR::StructField* f : *strct->fields) {
                const IR::Type* tplField = tpl->components->at(index);
                const IR::Type* destt = f->type;

                bool success = unify(errorPosition, destt, tplField, reportErrors);
                if (!success)
                    return false;
                index++;
            }
            return true;
        }

        if (reportErrors)
            ::error("%1%: Cannot unify %2% to %3%",
                    errorPosition, src->toString(), dest->toString());
        return false;
    } else if (dest->is<IR::Type_Base>()) {
        if (!src->is<IR::Type_Base>()) {
            if (reportErrors)
                ::error("%1%: Cannot unify %2% to %3%",
                        errorPosition, src->toString(), dest->toString());
            return false;
        }
        bool success = src == dest;
        if (!success) {
            if (reportErrors)
                ::error("%1%: Cannot unify %2% to %3%",
                        errorPosition, src->toString(), dest->toString());
            return false;
        }

        return true;
    } else if (dest->is<IR::Type_Declaration>() && src->is<IR::Type_Declaration>()) {
        bool canUnify = typeid(dest) == typeid(src) &&
                dest->to<IR::Type_Declaration>()->name == src->to<IR::Type_Declaration>()->name;
        if (!canUnify && reportErrors)
            ::error("%1%: Cannot unify %2% to %3%",
                    errorPosition, src->toString(), dest->toString());
        return canUnify;
    }

    if (reportErrors)
        ::error("%1%: Cannot unify %2% to %3%", errorPosition, src, dest);
    return false;
}

}  // namespace P4
