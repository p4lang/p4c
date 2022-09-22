/*
Copyright 2022 Intel Corp.

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

#include "flattenUnions.h"

namespace P4 {
const IR::MethodCallStatement*
HandleValidityHeaderUnion::processValidityForStr(const IR::Statement *s, const IR::Member *m,
                                                 cstring headerElement, cstring setValid) {
    auto method = new IR::Member(s->srcInfo, new IR::Member(m->expr,IR::ID(headerElement)),
                                 IR::ID(setValid));
    auto mc = new IR::MethodCallExpression(s->srcInfo, method,
                                           new IR::Vector<IR::Argument>());
    typeMap->setType(method, new IR::Type_Method(IR::Type_Void::get(),
                     new IR::ParameterList(), setValid));
    return new IR::MethodCallStatement(mc->srcInfo, mc);
}

const IR::Node* HandleValidityHeaderUnion::setInvalidforRest(const IR::Statement *s,
                                           const IR::Member *m, const IR::Type_HeaderUnion *hu,
                                           cstring exclude, bool setValidforCurrMem) {
    auto code_block = new IR::IndexedVector<IR::StatOrDecl>;
    code_block->push_back(s);
    if (setValidforCurrMem) {
        code_block->push_back(processValidityForStr(s, m, exclude, IR::Type_Header::setValid));
    }

    for (auto sfu : hu->fields) {
        if (exclude != sfu->name.name) {  // setInvalid for rest of the fields
            code_block->push_back(processValidityForStr(s, m, sfu->name,
                                                        IR::Type_Header::setInvalid));
        }
    }
    return new IR::BlockStatement(*code_block);
}

const IR::Node * HandleValidityHeaderUnion::expandIsValid(
                 const IR::Statement *a, const IR::MethodCallExpression *mce) {
    auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
    if (auto bim = mi->to<P4::BuiltInMethod>()) {
        if (bim->name == "isValid") {   // hdr.u.isValid() or u.isValid
            if (auto huType = bim->appliedTo->type->to<IR::Type_HeaderUnion>()) {  // u or hdr.u
                cstring tmp = refMap->newName("tmp");
                IR::PathExpression *tmpVar = new IR::PathExpression(IR::ID(tmp));
                toInsert.push_back(new IR::Declaration_Variable(
                                   IR::ID(tmp), IR::Type_Bits::get(32)));
                auto code_block = new IR::IndexedVector<IR::StatOrDecl>;
                code_block->push_back(new IR::AssignmentStatement(a->srcInfo,
                                      tmpVar, new IR::Constant(IR::Type_Bits::get(32), 0)));
                for (auto sfu : huType->fields) {
                    auto method = new IR::Member(a->srcInfo,
                                                  new IR::Member(bim->appliedTo, sfu->name),
                                                  IR::ID(IR::Type_Header::isValid));
                    auto mc = new IR::MethodCallExpression(a->srcInfo, method,
                                                           new IR::Vector<IR::Argument>());
                    typeMap->setType(method, new IR::Type_Method(IR::Type_Void::get(),
                                     new IR::ParameterList(), IR::Type_Header::isValid));
                    auto addOp = new IR::Add(a->srcInfo, tmpVar,
                                             new IR::Constant(IR::Type_Bits::get(32), 1));
                    auto assn = new IR::AssignmentStatement(a->srcInfo, tmpVar, addOp);
                    code_block->push_back(new IR::IfStatement(a->srcInfo, mc, assn, nullptr));
                }
                auto cond = new IR::Equ(a->srcInfo, tmpVar,
                                        new IR::Constant(IR::Type_Bits::get(32), 1));
                if (auto  assn = a->to<IR::AssignmentStatement>())
                    code_block->push_back(new IR::AssignmentStatement(a->srcInfo,
                                                                      assn->left, cond));
                else if (auto ifs = a->to<IR::IfStatement>())
                    code_block->push_back(new IR::IfStatement(a->srcInfo,
                                                              cond, ifs->ifTrue, ifs->ifFalse));
                else if (auto switchs = a->to<IR::SwitchStatement>()) {
                    code_block->push_back(new IR::SwitchStatement(a->srcInfo,
                                                                  cond, switchs->cases));
                }
                return new IR::BlockStatement(*code_block);
            }
        }
    }
    return a;
}

// u.h1 = {elem1, elem2....} => Already simplified to elementwise initialization
// u = u1 => Already simplified to elementwise copy
const IR::Node* HandleValidityHeaderUnion::postorder(IR::AssignmentStatement* a) {
    auto left = a->left;
    auto right = a->right;
    if (auto mce = right->to<IR::MethodCallExpression>()) {
        // a = u.isValid() or <hdr/m>.u.isValid
        return expandIsValid(a, mce);
    } else if (auto lhs = left->to<IR::Member>()) {
        if (auto huType = lhs->expr->type->to<IR::Type_HeaderUnion>()) {
            auto rhs = right->to<IR::Member>();
            if (right->type->is<IR::Type_Header>() ||
                (rhs && rhs->expr->type->is<IR::Type_HeaderUnion>())) {
                //  u.h1 = my_h1 or u.h1 = u1.h1
                auto isValid = new IR::Member(right->srcInfo, right,
                                  IR::ID(IR::Type_Header::isValid));
                auto result = new IR::MethodCallExpression(right->srcInfo, IR::Type::Boolean::get(),
                                                       isValid);
                typeMap->setType(isValid, new IR::Type_Method(IR::Type::Boolean::get(),
                                    new IR::ParameterList(), IR::Type_Header::isValid));

                auto trueBlock = setInvalidforRest(a, lhs, huType, lhs->member.name, true);
                auto method1 = new IR::Member(left->srcInfo, left,
                                                  IR::ID(IR::Type_Header::setInvalid));
                auto mc1 = new IR::MethodCallExpression(left->srcInfo, method1,
                              new IR::Vector<IR::Argument>());
                typeMap->setType(method1, new IR::Type_Method(IR::Type_Void::get(),
                                    new IR::ParameterList(), IR::Type_Header::setInvalid));
                auto ifFalse = new IR::MethodCallStatement(mc1->srcInfo, mc1);
                auto ifStatement = new IR::IfStatement(a->srcInfo, result,
                                           trueBlock->to<IR::BlockStatement>(), ifFalse);
                return ifStatement;
            }
        } else if (auto leftMem = lhs->expr->to<IR::Member>()) {  // hdr.u1.h1 or u1.h1
                // Handling hdr.u1.h1.data = <constant>
            if (auto huType = leftMem->expr->type->to<IR::Type_HeaderUnion>()) {
                // hdr.u1 or u1
                if (leftMem->type->is<IR::Type_Header>() && right->is<IR::Constant>()) {
                    // h1.data = constant
                    return setInvalidforRest(a, leftMem, huType, leftMem->member.name, true);
                }
            }
        }
    }
    return a;
}

const IR::Node* HandleValidityHeaderUnion::postorder(IR::IfStatement *a) {
    if (auto mce = a->condition->to<IR::MethodCallExpression>()) {
        return expandIsValid(a, mce);
     }
    return a;
}

const IR::Node* HandleValidityHeaderUnion::postorder(IR::SwitchStatement* a) {
    if (auto mce = a->expression->to<IR::MethodCallExpression>()) {
        return expandIsValid(a, mce);
    }
    return a;
}

// Assumes nested structs are flattened before this pass
const IR::Node* HandleValidityHeaderUnion::postorder(IR::MethodCallStatement* mcs) {
    auto mi = P4::MethodInstance::resolve(mcs->methodCall, refMap, typeMap);
    if (auto a = mi->to<P4::BuiltInMethod>()) {
        if (a->name == "setValid") {   // hdr.u.h1.setValid() or u.h1.setValid
            if (auto m = a->appliedTo->to<IR::Member>()) {
                if (auto huType = m->expr->type->to<IR::Type_HeaderUnion>())  // u.h1  or hdr.u.h1
                    return setInvalidforRest(mcs, m, huType, m->member.name, false);
            }
        }
    }
    return mcs;
}

const IR::Node* HandleValidityHeaderUnion::postorder(IR::P4Action* action) {
    if (toInsert.empty())
        return action;
    auto body = new IR::BlockStatement(action->body->srcInfo);
    for (auto a : toInsert)
        body->push_back(a);
    for (auto s : action->body->components)
        body->push_back(s);
    action->body = body;
    toInsert.clear();
    return action;
}

const IR::Node* HandleValidityHeaderUnion::postorder(IR::Function* function) {
    if (toInsert.empty())
        return function;
    auto body = new IR::BlockStatement(function->body->srcInfo);
    for (auto a : toInsert)
        body->push_back(a);
    for (auto s : function->body->components)
        body->push_back(s);
    function->body = body;
    toInsert.clear();
    return function;
}

const IR::Node* HandleValidityHeaderUnion::postorder(IR::P4Parser* parser) {
    if (toInsert.empty())
        return parser;
    parser->parserLocals.append(toInsert);
    toInsert.clear();
    return parser;
}

const IR::Node* HandleValidityHeaderUnion::postorder(IR::P4Control* control) {
    if (toInsert.empty())
        return control;
    control->controlLocals.append(toInsert);
    toInsert.clear();
    return control;
}

bool DoFlattenHeaderUnion::hasHeaderUnionField(IR::Type_Struct* s) {
    for (auto sf : s->fields) {
        auto ftype = typeMap->getType(sf, true);
        if (ftype->is<IR::Type_HeaderUnion>()) {
            return true;
        }
    }
    return false;
}

const IR::Node* DoFlattenHeaderUnion::postorder(IR::Type_Struct* s) {
    if (hasHeaderUnionField(s)) {
        IR::IndexedVector<IR::StructField> fields;
        for (auto sf : s->fields) {
            auto ftype = typeMap->getType(sf, true);
            if (ftype->is<IR::Type_HeaderUnion>()) {
                std::map <cstring, cstring> fieldMap;
                for (auto sfu : ftype->to<IR::Type_HeaderUnion>()->fields) {
                    cstring uName = refMap->newName(sf->name.name + "_" + sfu->name.name);
                    auto uType = sfu->type->getP4Type();
                    fieldMap.emplace(sfu->name.name, uName);
                    fields.push_back(new IR::StructField(IR::ID(uName), uType));
                }
                replacementMap.emplace(sf->name.name, fieldMap);
            } else {
                fields.push_back(sf);
            }
        }
        return new IR::Type_Struct(s->name, s->annotations, fields);
    }
    return s;
}

const IR::Node* DoFlattenHeaderUnion::postorder(IR::Declaration_Variable *dv) {
    auto ftype = typeMap->getTypeType(dv->type, true);
    if (ftype->is<IR::Type_HeaderUnion>()) {
        std::map <cstring, cstring> fieldMap;
        IR::IndexedVector<IR::Declaration> toInsert;
        for (auto sfu : ftype->to<IR::Type_HeaderUnion>()->fields) {
            cstring uName = refMap->newName(dv->name.name + "_" + sfu->name.name);
            auto uType =  sfu->type->getP4Type();
            fieldMap.emplace(sfu->name.name, uName);
            toInsert.push_back(new IR::Declaration_Variable(IR::ID(uName), uType));
        }
        replacementMap.emplace(dv->name.name, fieldMap);
        replaceDVMap.emplace(dv, toInsert);
    }
    return dv;
}

const IR::Node* DoFlattenHeaderUnion::postorder(IR::Member* m) {
    if (m->expr->type->to<IR::Type_HeaderUnion>()) {
        if (auto huf = m->expr->to<IR::Member>()) {
            if (replacementMap.count(huf->member.name)) {
                if (replacementMap.at(huf->member.name).count(m->member.name)) {
                    auto newHuName = replacementMap.at(huf->member.name).at(m->member.name);
                    return new IR::Member(huf->expr,IR::ID(newHuName));
                }
            }
            return m;
        } else if (auto huf = m->expr->to<IR::PathExpression>()) {
            if (replacementMap.count(huf->path->name.name)) {
                if (replacementMap.at(huf->path->name.name).count(m->member.name)) {
                    auto newHuName = replacementMap.at(huf->path->name.name).at(m->member.name);
                    return new IR::PathExpression(IR::ID(newHuName));
                }
            }
            return m;
        }
    }
    return m;
}
const IR::Node* DoFlattenHeaderUnion::postorder(IR::P4Action* action) {
    auto actiondecls = action->body->components;
    for (auto rdv : replaceDVMap) {
        if (auto decl = action->getDeclByName(rdv.first->name.name)) {
            for (auto it = actiondecls.begin();it != actiondecls.end(); it++) {
                auto d = *it;
                if (d->is<IR::Declaration>() && decl->is<IR::Declaration>()) {
                    auto dName = d->to<IR::Declaration>()->name.name;
                    auto declName = decl->to<IR::Declaration>()->name.name;
                    if (dName == declName) {
                        actiondecls.insert(it, rdv.second.begin(), rdv.second.end());
                    }
                }
            }
        }
    }
    auto body = new IR::BlockStatement(action->body->srcInfo);
    for (auto a : actiondecls)
        body->push_back(a);
    action->body = body;
    return action;
}

const IR::Node* DoFlattenHeaderUnion::postorder(IR::Function* function) {
    auto functiondecls = function->body->components;
    for (auto rdv : replaceDVMap) {
        if (auto decl = function->getDeclByName(rdv.first->name.name)) {
            for (auto it = functiondecls.begin();it != functiondecls.end(); it++) {
                auto d = *it;
                if (d->is<IR::Declaration>() && decl->is<IR::Declaration>()) {
                    auto dName = d->to<IR::Declaration>()->name.name;
                    auto declName = decl->to<IR::Declaration>()->name.name;
                    if (dName == declName) {
                        functiondecls.insert(it, rdv.second.begin(), rdv.second.end());
                    }
                }
            }
        }
    }
    auto body = new IR::BlockStatement(function->body->srcInfo);
    for (auto a : functiondecls)
        body->push_back(a);
    function->body = body;
    return function;
}

const IR::Node* DoFlattenHeaderUnion::postorder(IR::P4Parser* parser) {
    auto parserdecls = parser->parserLocals;
    for (auto rdv : replaceDVMap) {
        if (auto decl = parser->getDeclByName(rdv.first->name.name)) {
            auto it = std::find(parserdecls.begin(), parserdecls.end(), decl);
            if (it != parserdecls.end()) {
                parserdecls.insert(it, rdv.second.begin(), rdv.second.end());
            }
        }
    }
    parser->parserLocals = parserdecls;

    return parser;
}

const IR::Node* DoFlattenHeaderUnion::postorder(IR::P4Control* control) {
    auto controldecls = control->controlLocals;
    for (auto rdv : replaceDVMap) {
        if (auto decl = control->getDeclByName(rdv.first->name.name)) {
            auto it = std::find(controldecls.begin(), controldecls.end(), decl);
            if (it != controldecls.end()) {
                controldecls.insert(it, rdv.second.begin(), rdv.second.end());
            }
        }
    }
    control->controlLocals = controldecls;
    return control;
}
}  // namespace P4
