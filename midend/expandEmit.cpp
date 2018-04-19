/*
Copyright 2017 VMware, Inc.

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

#include "expandEmit.h"
#include "frontends/p4/coreLibrary.h"

namespace P4 {

bool DoExpandEmit::expandArg(
    const IR::Type* type, const IR::Argument* arg,
    std::vector<const IR::Argument*> *result, std::vector<const IR::Type*> *resultTypes) {
    if (type->is<IR::Type_Header>()) {
        result->push_back(arg);
        resultTypes->push_back(type);
        return false;
    } else if (auto st = type->to<IR::Type_Stack>()) {
        int size = st->getSize();
        for (int i = 0; i < size; i++) {
            auto index = new IR::Constant(i);
            auto element = new IR::Argument(
                arg->srcInfo, arg->name, new IR::ArrayIndex(arg->expression, index));
            result->push_back(element);
            resultTypes->push_back(st->elementType);
        }
        return true;
    } else if (auto tup = type->to<IR::Type_Tuple>()) {
        auto le = arg->expression->to<IR::ListExpression>();
        BUG_CHECK(le != nullptr && le->size() == tup->size(), "%1%: not a list?", arg);
        for (size_t i = 0; i < le->size(); i++) {
            auto expr = new IR::Argument(arg->srcInfo, arg->name, le->components.at(i));
            auto type = tup->components.at(i);
            expandArg(type, expr, result, resultTypes);
        }
        return true;
    } else {
        BUG_CHECK(type->is<IR::Type_StructLike>(),
                  "%1%: expected a struct or header_union type", type);
        auto strct = type->to<IR::Type_StructLike>();
        for (auto f : strct->fields) {
            auto expr = new IR::Argument(
                arg->srcInfo, arg->name, new IR::Member(arg->expression, f->name));
            auto type = typeMap->getTypeType(f->type, true);
            expandArg(type, expr, result, resultTypes);
        }
        return true;
    }
}

const IR::Node* DoExpandEmit::postorder(IR::MethodCallStatement* statement) {
    auto mi = MethodInstance::resolve(statement->methodCall, refMap, typeMap);
    if (auto em = mi->to<P4::ExternMethod>()) {
        if (em->originalExternType->name.name == P4::P4CoreLibrary::instance.packetOut.name &&
            em->method->name.name == P4::P4CoreLibrary::instance.packetOut.emit.name) {
            if (em->expr->arguments->size() != 1) {
                ::error("%1%: expected exactly 1 argument", statement);
                return statement;
            }

            auto arg0 = em->expr->arguments->at(0);
            auto type = typeMap->getType(arg0, true);
            std::vector<const IR::Argument*> expansion;
            std::vector<const IR::Type*> expansionTypes;
            if (expandArg(type, arg0, &expansion, &expansionTypes)) {
                auto vec = new IR::IndexedVector<IR::StatOrDecl>();
                auto it = expansionTypes.begin();
                for (auto e : expansion) {
                    auto method = statement->methodCall->method->clone();
                    auto argType = *it;
                    auto args = new IR::Vector<IR::Argument>();
                    args->push_back(e);
                    auto typeArgs = new IR::Vector<IR::Type>();
                    typeArgs->push_back(argType->getP4Type());
                    auto mce = new IR::MethodCallExpression(
                        statement->methodCall->srcInfo, method, typeArgs, args);
                    auto stat = new IR::MethodCallStatement(mce);
                    vec->push_back(stat);
                    ++it;
                }
                return new IR::BlockStatement(*vec);
            }
        }
    }
    return statement;
}


}  // namespace P4
