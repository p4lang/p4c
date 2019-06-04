/*
Copyright 2016 VMware, Inc.

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

#include "expandLookahead.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/coreLibrary.h"

namespace P4 {

void DoExpandLookahead::expandSetValid(const IR::Expression* base, const IR::Type* type,
                                       IR::IndexedVector<IR::StatOrDecl>* output) {
    if (type->is<IR::Type_Struct>()) {
        auto st = type->to<IR::Type_Struct>();
        for (auto f : st->fields) {
            auto t = typeMap->getTypeType(f->type, true);
            if (t == nullptr)
                return;
            auto mem = new IR::Member(base, f->name);
            expandSetValid(mem, t, output);
        }
    } else if (type->is<IR::Type_Header>()) {
        auto setValid = new IR::Member(base, IR::Type_Header::setValid);
        auto mc = new IR::MethodCallExpression(setValid);
        output->push_back(new IR::MethodCallStatement(mc));
    }
}

const IR::Expression* DoExpandLookahead::expand(
    const IR::PathExpression* base, const IR::Type* type, unsigned* offset) {
    if (type->is<IR::Type_Struct>() || type->is<IR::Type_Header>()) {
        auto vec = new IR::IndexedVector<IR::NamedExpression>();
        auto st = type->to<IR::Type_StructLike>();
        for (auto f : st->fields) {
            auto t = typeMap->getTypeType(f->type, true);
            if (t == nullptr)
                continue;
            auto e = expand(base, t, offset);
            vec->push_back(new IR::NamedExpression(f->srcInfo, f->name, e));
        }
        auto type = st->getP4Type()->to<IR::Type_Name>();
        return new IR::StructInitializerExpression(
            base->srcInfo, type, type, *vec);
    } else if (type->is<IR::Type_Bits>() || type->is<IR::Type_Boolean>()) {
        unsigned size = type->width_bits();
        BUG_CHECK(size > 0, "%1%: unexpected size %2%", type, size);
        auto expression = new IR::Slice(base->clone(), *offset - 1, *offset - size);
        *offset -= size;
        return expression;
    } else {
        ::error("%1%: unexpected type in lookahead argument", type);
        return nullptr;
    }
}

DoExpandLookahead::ExpansionInfo*
DoExpandLookahead::convertLookahead(const IR::MethodCallExpression* expression) {
    if (expression == nullptr)
        return nullptr;
    auto mi = MethodInstance::resolve(expression, refMap, typeMap);
    if (!mi->is<P4::ExternMethod>())
        return nullptr;
    auto em = mi->to<P4::ExternMethod>();
    if (em->originalExternType->name != P4CoreLibrary::instance.packetIn.name ||
        em->method->name != P4CoreLibrary::instance.packetIn.lookahead.name)
        return nullptr;

    // this is a call to packet_in.lookahead.
    BUG_CHECK(expression->typeArguments->size() == 1,
              "Expected 1 type parameter for %1%", em->method);
    auto targ = expression->typeArguments->at(0);
    auto typearg = typeMap->getTypeType(targ, true);
    if (!typearg->is<IR::Type_StructLike>() && !typearg->is<IR::Type_Tuple>())
        return nullptr;

    int width = typeMap->minWidthBits(typearg, expression);
    if (width < 0)
        return nullptr;

    auto bittype = IR::Type_Bits::get(width);
    auto name = refMap->newName("tmp");
    auto decl = new IR::Declaration_Variable(IR::ID(name), bittype, nullptr);
    newDecls.push_back(decl);

    auto ta = new IR::Vector<IR::Type>();
    ta->push_back(bittype);
    auto mc = new IR::MethodCallExpression(
        expression->srcInfo, expression->method->clone(), ta, expression->arguments);
    auto pathe = new IR::PathExpression(name);
    auto lookupCall = new IR::AssignmentStatement(expression->srcInfo, pathe, mc);
    auto result = new ExpansionInfo;
    result->statement = lookupCall;
    result->width = (unsigned)width;
    result->origType = typearg;
    result->tmp = pathe;
    return result;
}

const IR::Node* DoExpandLookahead::postorder(IR::MethodCallStatement* statement) {
    auto ei = convertLookahead(statement->methodCall);
    if (ei == nullptr)
        return statement;
    return ei->statement;
}

const IR::Node* DoExpandLookahead::postorder(IR::AssignmentStatement* statement) {
    if (!statement->right->is<IR::MethodCallExpression>())
        return statement;

    auto ei = convertLookahead(statement->right->to<IR::MethodCallExpression>());
    if (ei == nullptr)
        return statement;
    auto result = new IR::BlockStatement;
    result->push_back(ei->statement);

    expandSetValid(statement->left->clone(), ei->origType, &result->components);
    auto init = expand(ei->tmp->clone(), ei->origType, &ei->width);
    if (init == nullptr)
        return statement;
    auto assignment = new IR::AssignmentStatement(statement->srcInfo, statement->left, init);
    result->push_back(assignment);
    return result;
}

}  // namespace P4
