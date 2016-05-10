#include "methodInstance.h"
#include "ir/ir.h"

namespace P4 {

// If useExpressionType is true trust the type in mce->type
MethodInstance*
MethodInstance::resolve(const IR::MethodCallExpression* mce, const P4::ReferenceMap* refMap,
                        const P4::TypeMap* typeMap, bool useExpressionType) {
    auto mt = typeMap->getType(mce->method);
    if (mt == nullptr && useExpressionType)
        mt = mce->method->type;
    CHECK_NULL(mt);
    // mt can be Type_Method or Type_Action
    if (mce->method->is<IR::Member>()) {
        auto mem = mce->method->to<IR::Member>();
        auto basetype = typeMap->getType(mem->expr);
        if (basetype == nullptr && useExpressionType)
            basetype = mem->expr->type;
        if (basetype->is<IR::Type_Header>()) {
            if (mem->member == IR::Type_Header::setValid ||
                mem->member == IR::Type_Header::setInvalid ||
                mem->member == IR::Type_Header::isValid)
                return new BuiltInMethod(mce, mem->member, mem->expr);
        }
        if (basetype->is<IR::Type_Stack>()) {
            if (mem->member == IR::Type_Stack::push_front ||
                mem->member == IR::Type_Stack::pop_front)
                return new BuiltInMethod(mce, mem->member, mem->expr);
        }
        if (mem->expr->is<IR::PathExpression>()) {
            auto pe = mem->expr->to<IR::PathExpression>();
            auto decl = refMap->getDeclaration(pe->path, true);
            auto type = typeMap->getType(decl->getNode());
            if (type->is<IR::Type_SpecializedCanonical>())
                type = type->to<IR::Type_SpecializedCanonical>()->substituted;
            BUG_CHECK(type != nullptr, "Could not resolve type for %1%", decl);
            if (type->is<IR::IApply>() &&
                mem->member == IR::IApply::applyMethodName) {
                return new ApplyMethod(mce, decl, type->to<IR::IApply>());
            }
            if (type->is<IR::Type_Extern>()) {
                auto et = type->to<IR::Type_Extern>();
                auto methodType = mt->to<IR::Type_Method>();
                CHECK_NULL(methodType);
                auto method = et->lookupMethod(mem->member, mce->arguments->size());
                return new ExternMethod(mce, decl, method, et, methodType);
            }
        }
    } else if (mce->method->is<IR::PathExpression>()) {
        auto pe = mce->method->to<IR::PathExpression>();
        auto decl = refMap->getDeclaration(pe->path, true);
        if (decl->is<IR::Method>()) {
            auto methodType = mt->to<IR::Type_Method>();
            CHECK_NULL(methodType);
            return new ExternFunction(mce, decl->to<IR::Method>(), methodType);
        } else if (decl->is<IR::P4Action>()) {
            return new ActionCall(mce, decl->to<IR::P4Action>());
        }
    }

    BUG("Unexpected method call %1%", mce);
    return nullptr;  // unreachable
}

ConstructorCall*
ConstructorCall::resolve(const IR::ConstructorCallExpression* cce,
                         const P4::TypeMap* typeMap) {
    auto t = typeMap->getType(cce, true);
    if (t->is<IR::Type_Extern>())
        return new ExternConstructorCall(cce, t->to<IR::Type_Extern>());
    else if (t->is<IR::IContainer>())
        return new ContainerConstructorCall(cce, t->to<IR::IContainer>());
    BUG("Unexpected constructor call %1%", cce);
}

}  // namespace P4
