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

#include "expandMirror.h"
#include "frontends/p4/coreLibrary.h"


namespace P4 {

void ExpandMirror::explode(const IR::Expression* expression, IR::Vector<IR::Expression>* output) {
    if (auto st = expression->type->to<IR::Type_Header>()) {
        for (auto f : st->fields) {
            auto e = new IR::Member(f->type, expression, f->name);
            explode(e, output);
        }
    } else {
        BUG_CHECK(!expression->type->is<IR::Type_StructLike>() && !expression->type->is<IR::Type_Stack>(),
                  "%1%: unexpected type", expression->type);
        output->push_back(expression);
    }
}

void ExpandMirror::postorder(IR::MethodCallExpression* expr) {
    auto mi = MethodInstance::resolve(expr, refMap, typeMap);
    auto em = mi->to<P4::ExternMethod>();
    if (!em) {
      return;
    }
    if (em->originalExternType->name.name != "Mirror") {
      return;
    }
    if (expr->arguments->size() != 2) {
      ::error("%1%: expected exactly 2 arguments", expr);
      return;
    }
    auto arg1 = expr->arguments->at(1);
    auto type = typeMap->getType(arg1, true);
    if (!type->is<IR::Type_Header>()) {
      return;
    }
    auto result = new IR::ListExpression(arg1->srcInfo, {});
    explode(arg1->expression, &result->components);
    auto args = expr->arguments->clone();
    args->at(1) = new IR::Argument(arg1->name, result);
    expr->arguments = args;
}


} // namespace P4
