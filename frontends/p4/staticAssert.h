/*
Copyright 2022 VMware, Inc.

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

#ifndef FRONTENDS_P4_STATICASSERT_H_
#define FRONTENDS_P4_STATICASSERT_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace p4c::P4 {

using namespace literals;

/**
 * Evaluates static_assert invocations.
 * A successful assertion is constant-folded to 'true'.
 */
class DoStaticAssert : public Transform, public ResolutionContext {
    TypeMap *typeMap;
    bool removeStatement = false;

    // Cannot go static here as cstring is not constexpr
    const cstring staticAssertMethodName = "static_assert"_cs;

 public:
    explicit DoStaticAssert(TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("DoStaticAssert");
    }
    const IR::Node *postorder(IR::MethodCallExpression *method) override {
        MethodInstance *mi = MethodInstance::resolve(method, this, typeMap);
        if (auto ef = mi->to<ExternFunction>()) {
            if (ef->method->name == staticAssertMethodName) {
                auto subst = ef->substitution;
                auto params = subst.getParametersInOrder();
                if (!params->moveNext()) {
                    ::warning(ErrorType::WARN_INVALID, "static_assert with no arguments: %1%",
                              method);
                    return method;
                }
                auto param = params->getCurrent();
                CHECK_NULL(param);
                auto arg = subst.lookup(param);
                CHECK_NULL(arg);
                if (auto bl = arg->expression->to<IR::BoolLiteral>()) {
                    if (!bl->value) {
                        std::string_view message = "static_assert failed";
                        if (params->moveNext()) {
                            param = params->getCurrent();
                            CHECK_NULL(param);
                            auto msg = subst.lookup(param);
                            CHECK_NULL(msg);
                            if (const auto *sl = msg->expression->to<IR::StringLiteral>()) {
                                message = sl->value.string_view();
                            }
                        }
                        ::error(ErrorType::ERR_EXPECTED, "%1%: %2%", method, message);
                        return method;
                    }
                    if (getContext()->node->is<IR::MethodCallStatement>()) {
                        removeStatement = true;
                        return method;
                    }
                    return new IR::BoolLiteral(method->srcInfo, true);
                } else {
                    ::error(ErrorType::ERR_UNEXPECTED,
                            "Could not evaluate static_assert to a constant: %1%", arg);
                    return method;
                }
            }
        }
        return method;
    }

    const IR::Node *postorder(IR::MethodCallStatement *statement) override {
        if (removeStatement) {
            removeStatement = false;
            return nullptr;
        }
        return statement;
    }
};

class StaticAssert : public PassManager {
 public:
    explicit StaticAssert(TypeMap *typeMap) {
        passes.push_back(new TypeInference(typeMap));
        passes.push_back(new DoStaticAssert(typeMap));
        setName("StaticAssert");
    }
};

}  // namespace p4c::P4

#endif /* FRONTENDS_P4_STATICASSERT_H_ */
