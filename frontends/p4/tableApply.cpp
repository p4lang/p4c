#include "tableApply.h"
#include "methodInstance.h"

namespace P4 {

const IR::P4Table*
TableApplySolver::isHit(const IR::Expression* expression,
                        const P4::ReferenceMap* refMap,
                        const P4::TypeMap* typeMap) {
    if (!expression->is<IR::Member>())
        return nullptr;
    auto mem = expression->to<IR::Member>();
    if (!mem->expr->is<IR::MethodCallExpression>())
        return nullptr;
    if (mem->member != IR::Type_Table::hit)
        return nullptr;
    auto mce = mem->expr->to<IR::MethodCallExpression>();
    auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
    if (!mi->isApply())
        return nullptr;

    auto am = mi->to<P4::ApplyMethod>();
    if (!am->object->is<IR::P4Table>())
        return nullptr;
    return am->object->to<IR::P4Table>();
}

const IR::P4Table*
TableApplySolver::isActionRun(const IR::Expression* expression,
                              const P4::ReferenceMap* refMap,
                              const P4::TypeMap* typeMap) {
    auto mem = expression->to<IR::Member>();
    if (mem == nullptr)
        return nullptr;
    if (mem->member.name != IR::Type_Table::action_run)
        return nullptr;
    auto mce = mem->expr->to<IR::MethodCallExpression>();
    if (mce == nullptr)
        return nullptr;
    auto instance = P4::MethodInstance::resolve(mce, refMap, typeMap);
    if (!instance->is<P4::ApplyMethod>())
        return nullptr;
    auto am = instance->to<P4::ApplyMethod>();
    if (!am->object->is<IR::P4Table>())
        return nullptr;
    return am->object->to<IR::P4Table>();
}

}  // namespace P4
