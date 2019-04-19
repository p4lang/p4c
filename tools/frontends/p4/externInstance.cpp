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

#include "externInstance.h"

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {


boost::optional<ExternInstance>
ExternInstance::resolve(const IR::Expression* expr,
                        ReferenceMap* refMap,
                        TypeMap* typeMap,
                        const boost::optional<cstring>& defaultName
                            /* = boost::none */) {
    CHECK_NULL(expr);
    CHECK_NULL(refMap);
    CHECK_NULL(typeMap);

    if (expr->is<IR::PathExpression>()) {
        return resolve(expr->to<IR::PathExpression>(), refMap, typeMap);
    } else if (expr->is<IR::ConstructorCallExpression>()) {
        return resolve(expr->to<IR::ConstructorCallExpression>(),
                       refMap, typeMap, defaultName);
    } else {
        return boost::none;
    }
}

boost::optional<ExternInstance>
ExternInstance::resolve(const IR::PathExpression* path,
                        ReferenceMap* refMap,
                        TypeMap* typeMap) {
    CHECK_NULL(path);
    CHECK_NULL(refMap);
    CHECK_NULL(typeMap);

    auto decl = refMap->getDeclaration(path->path, true);
    if (!decl->is<IR::Declaration_Instance>()) return boost::none;

    auto instance = decl->to<IR::Declaration_Instance>();
    auto type = typeMap->getType(instance);
    if (!type) {
        BUG("Couldn't determine the type of expression: %1%", path);
        return boost::none;
    }

    auto instantiation = Instantiation::resolve(instance, refMap, typeMap);
    if (!instantiation->is<ExternInstantiation>()) return boost::none;
    auto externInstantiation = instantiation->to<ExternInstantiation>();

    return ExternInstance{instance->controlPlaneName(), path,
                          externInstantiation->type,
                          instance->arguments,
                          externInstantiation->substitution,
                          instance->to<IR::IAnnotated>()};
}

boost::optional<ExternInstance>
ExternInstance::resolve(const IR::ConstructorCallExpression* constructorCallExpr,
                        ReferenceMap* refMap,
                        TypeMap* typeMap,
                        const boost::optional<cstring>& name /* = boost::none */) {
    CHECK_NULL(constructorCallExpr);
    CHECK_NULL(refMap);
    CHECK_NULL(typeMap);

    auto constructorCall =
            P4::ConstructorCall::resolve(constructorCallExpr, refMap, typeMap);
    if (!constructorCall->is<P4::ExternConstructorCall>()) return boost::none;

    auto type = constructorCall->to<P4::ExternConstructorCall>()->type;
    return ExternInstance{name, constructorCallExpr, type,
                          constructorCallExpr->arguments,
                          constructorCall->substitution,
                          constructorCallExpr->to<IR::IAnnotated>()};
}

}  // namespace P4
