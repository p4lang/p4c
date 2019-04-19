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

#include "tableApply.h"
#include "methodInstance.h"

namespace P4 {

const IR::P4Table*
TableApplySolver::isHit(const IR::Expression* expression,
                        ReferenceMap* refMap,
                        TypeMap* typeMap) {
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
                              ReferenceMap* refMap,
                              TypeMap* typeMap) {
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
