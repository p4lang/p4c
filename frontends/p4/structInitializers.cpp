/*
Copyright 2018 VMware, Inc.

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
#include "structInitializers.h"

namespace P4 {
const IR::Expression*
convert(const IR::Expression* expression, const IR::Type* type);

const IR::Expression* defaultValue(const IR::Type* type, const Util::SourceInfo srcInfo) {
    IR::Expression* expr = nullptr;
    if (type->is<IR::Type_Bits>()) {
        expr = new IR::Constant(type, 0);
    } else if (type->is<IR::Type_Boolean>()) {
        expr = new IR::BoolLiteral(false);
    } else if (type->is<IR::Type_Error>()) {
        auto path = new IR::Path(IR::ID(srcInfo, "error"));
        auto typeName =  new IR::Type_Name(srcInfo, path);
        auto id = new IR::ID(srcInfo, "NoError");
        auto tne = new IR::TypeNameExpression(srcInfo, typeName);
        expr = new IR::Member(srcInfo, tne, *id);
    } else if (auto t = type->to<IR::Type_Enum>()) {
        BUG_CHECK(t->members.size(), "unknown default value for enum with no members");
        auto tne = new IR::TypeNameExpression(srcInfo,
                                               new IR::Type_Name(t->name));
        expr = new IR::Member(srcInfo, tne, t->members.at(0)->name);
    } else if (auto t = type->to<IR::Type_SerEnum>()) {
        expr = new IR::Constant(t->type, 0);
    } else if (type->is<IR::Type_StructLike>() || type->is<IR::Type_BaseList>()) {
        auto vec = new IR::Vector<IR::Expression>();
        auto conv = convert(new IR::ListExpression(srcInfo, *vec, true), type);
        return conv;
    } else {
        ::error(ErrorType::ERR_TYPE_ERROR,
                "%1%: Unknown default value for type %2%", srcInfo, type);
        expr = new IR::Constant(0);  // we just need expr not to be nullptr to avoid
        // compiler bug (e.g if passed as NamedExpression constructor argument)
    }
    return expr;
}

bool isDefaultInit(const IR::Expression* expr) {
    auto le = expr->to<IR::ListExpression>();
    auto se = expr->to<IR::StructExpression>();
    return ((le && le->defaultInitializer) || (se && se->defaultInitializer));
}

IR::MethodCallStatement* generateSetInvalid(const IR::Type* type, const Util::SourceInfo srcInfo,
                                            const IR::Expression* exp) {
    if (type->is<IR::Type_Header>()) {
        auto method = new IR::Member(srcInfo, exp,
                                     IR::Type_Header::setInvalid);
        auto mc = new IR::MethodCallExpression(srcInfo, method, new IR::Vector<IR::Argument>());
        return new IR::MethodCallStatement(srcInfo, mc);
    } else {
        auto hu = type->to<IR::Type_HeaderUnion>();
        auto hdr = hu->fields.at(0);
        auto left = new IR::Member(exp, hdr->name);
        auto method = new IR::Member(srcInfo, left,
                                     IR::Type_Header::setInvalid);
        auto mc = new IR::MethodCallExpression(srcInfo, method, new IR::Vector<IR::Argument>());
        return new IR::MethodCallStatement(mc->srcInfo, mc);
    }
}

// rexpr is struct expression or list expression used to assign a value to struct or tuple
// respectively; ltype is destination type
const IR::Expression* resolveDefaultInitTuple(const IR::Expression* rexpr,
                                              const IR::Type* ltype,
                                              IR::IndexedVector<IR::StatOrDecl>* retval,
                                              ReferenceMap* refMap) {
    if (auto tup = ltype->to<IR::Type_BaseList>()) {
        auto le = rexpr->to<IR::ListExpression>();
        auto vec = new IR::Vector<IR::Expression>();
        auto l = le->size();
        for (size_t index = 0; index < l; index++) {
            auto expr = le->components.at(index);
            auto type = tup->components.at(index);
            if ((type->is<IR::Type_Header>() &&
                 expr->to<IR::StructExpression>()->defaultInitializer)
                 || type->is<IR::Type_HeaderUnion>()) {
                cstring name = refMap->newName("hdr_or_hu");
                auto decl = new IR::Declaration_Variable(rexpr->srcInfo, IR::ID(name), type);
                IR::Expression* pe = new IR::PathExpression(rexpr->srcInfo,
                                                            type, new IR::Path(name));
                retval->push_back(decl);
                vec->push_back(pe);
                auto stat = generateSetInvalid(type, rexpr->srcInfo, pe);
                retval->push_back(stat);
            } else if (type->is<IR::Type_BaseList>() || type->is<IR::Type_Struct>()) {
                vec->push_back(resolveDefaultInitTuple(expr, type, retval, refMap));
            } else {
                vec->push_back(expr);
            }
        }
        auto result = new IR::ListExpression(rexpr->srcInfo, *vec);
        return result;
    } else if (auto strct = ltype->to<IR::Type_Struct>()) {
        auto sli = rexpr->to<IR::StructExpression>();
        auto si = new IR::IndexedVector<IR::NamedExpression>();
        for (auto f : strct->fields) {
            auto ne = sli->components.getDeclaration<IR::NamedExpression>(f->name.name);
            if ((f->type->is<IR::Type_Header>() &&
                 ne->expression->to<IR::StructExpression>()->defaultInitializer)
                 || f->type->is<IR::Type_HeaderUnion>()) {
                cstring name = refMap->newName("hdr_or_hu");
                auto decl = new IR::Declaration_Variable(rexpr->srcInfo, IR::ID(name), f->type);
                IR::Expression* pe = new IR::PathExpression(rexpr->srcInfo,
                                                            f->type, new IR::Path(name));
                ne = new IR::NamedExpression(rexpr->srcInfo, f->name, pe);
                retval->push_back(decl);
                auto stat = generateSetInvalid(f->type, rexpr->srcInfo, pe);
                retval->push_back(stat);

            } else if (f->type->is<IR::Type_BaseList>() || f->type->is<IR::Type_Struct>()) {
                ne = new IR::NamedExpression(rexpr->srcInfo, f->name,
                     resolveDefaultInitTuple(ne->expression, f->type, retval, refMap));
            }
            si->push_back(ne);
        }
        auto type = strct->getP4Type()->to<IR::Type_Name>();
        return new IR::StructExpression(rexpr->srcInfo, type, type, *si);
    }
    return nullptr;
}


const IR::Statement* resolveDefaultInit(IR::AssignmentStatement* statement, const IR::Type* ltype,
                                        IR::IndexedVector<IR::StatOrDecl>* retval,
                                        ReferenceMap* refMap) {
    auto strct = ltype->to<IR::Type_StructLike>();
    if (auto si = statement->right->to<IR::StructExpression>()) {
        for (auto f : strct->fields) {
            auto right = si->components.getDeclaration<IR::NamedExpression>(f->name);
            auto left = new IR::Member(statement->left, f->name);
            if ((f->type->is<IR::Type_Header>() && isDefaultInit(right->expression)) ||
                 f->type->is<IR::Type_HeaderUnion>()) {
                auto stat = generateSetInvalid(f->type, statement->srcInfo, left);
                retval->push_back(stat);
                continue;
            } else if (f->type->is<IR::Type_Struct>()) {
                resolveDefaultInit(new IR::AssignmentStatement(statement->srcInfo, left,
                                   right->expression), f->type, retval, refMap);
                continue;
            } else if (f->type->is<IR::Type_BaseList>()) {
                auto expr = resolveDefaultInitTuple(right->expression, f->type, retval, refMap);
                retval->push_back(new IR::AssignmentStatement(
                                  statement->srcInfo, left, expr));
            }
            retval->push_back(new IR::AssignmentStatement(
                              statement->srcInfo, left, right->expression));
        }
    }
    return new IR::BlockStatement(statement->srcInfo, *retval);
}

/// Given an expression and a destination type, convert ListExpressions
/// that occur within expression to StructExpression if the
/// destination type matches.
const IR::Expression*
convert(const IR::Expression* expression, const IR::Type* type) {
    bool modified = false;
    if (auto st = type->to<IR::Type_StructLike>()) {
        bool isHdr = type->is<IR::Type_Header>();
        bool hdrInvalid = false;
        auto si = new IR::IndexedVector<IR::NamedExpression>();
        if (auto le = expression->to<IR::ListExpression>()) {
            size_t index = 0;
            auto l = le->size();
            for (auto f : st->fields) {
                if (index >= l) {
                    break;
                }
                auto expr = le->components.at(index);
                auto conv = convert(expr, f->type);
                auto ne = new IR::NamedExpression(conv->srcInfo, f->name, conv);
                si->push_back(ne);
                index++;
            }
            if (le->defaultInitializer) {
                if (index == 0 && isHdr) {
                    hdrInvalid = true;
                }
                const IR::Expression* expr;
                modified = true;
                if (!hdrInvalid) {
                    for (; index < st->fields.size(); index++) {
                        auto f = st->fields.at(index);
                        expr = defaultValue(f->type, expression->srcInfo);
                        auto ne = new IR::NamedExpression(expression->srcInfo, f->name, expr);
                        si->push_back(ne);
                    }
                }
            }
            auto type = st->getP4Type()->to<IR::Type_Name>();
            auto result = new IR::StructExpression(
                expression->srcInfo, type, type, *si, hdrInvalid);
            return result;
        } else if (auto sli = expression->to<IR::StructExpression>()) {
            if (!sli->defaultInitializer) {
                for (auto f : st->fields) {
                    auto ne = sli->components.getDeclaration<IR::NamedExpression>(f->name.name);
                    BUG_CHECK(ne != nullptr, "%1%: no initializer for %2%", expression, f);
                    auto convNe = convert(ne->expression, f->type);
                    if (convNe != ne->expression)
                        modified = true;
                    ne = new IR::NamedExpression(ne->srcInfo, f->name, convNe);
                    si->push_back(ne);
                }
            } else {
                modified = true;
                for (auto f : st->fields) {
                    auto ne = sli->components.getDeclaration<IR::NamedExpression>(f->name.name);
                    if (ne == nullptr) {
                        const IR::Expression* expr;
                        expr = defaultValue(f->type, expression->srcInfo);
                        auto ne = new IR::NamedExpression(expression->srcInfo, f->name, expr);
                        si->push_back(ne);
                    } else {
                        auto convNe = convert(ne->expression, f->type);
                        if (convNe != ne->expression)
                            modified = true;
                        ne = new IR::NamedExpression(ne->srcInfo, f->name, convNe);
                        si->push_back(ne);
                    }
                }
            }
            if (modified || sli->type->is<IR::Type_UnknownStruct>()) {
                auto type = st->getP4Type()->to<IR::Type_Name>();
                auto result = new IR::StructExpression(
                    expression->srcInfo, type, type, *si);
                return result;
            }
        }
    } else if (auto tup = type->to<IR::Type_BaseList>()) {
        auto le = expression->to<IR::ListExpression>();
        if (le == nullptr)
            return expression;
        auto vec = new IR::Vector<IR::Expression>();
        size_t index = 0;
        auto l = le->size();
        for ( ; index < l; index++) {
            auto expr = le->components.at(index);
            auto type = tup->components.at(index);
            auto conv = convert(expr, type);
            vec->push_back(conv);
            modified |= (conv != expr);
        }
        if (le->defaultInitializer) {
            const IR::Expression* expr;
            modified = true;
            for ( ; index < tup->components.size(); index++) {
                auto f = tup->components.at(index);
                expr = defaultValue(f, expression->srcInfo);
                vec->push_back(expr);
            }
        }
        if (modified) {
            auto result = new IR::ListExpression(expression->srcInfo, *vec);
            return result;
        }
    }
    return expression;
}

const IR::Node* CreateStructInitializers::postorder(IR::MethodCallExpression* expression) {
    auto mi = MethodInstance::resolve(expression, refMap, typeMap);
    auto result = expression;
    auto convertedArgs = new IR::Vector<IR::Argument>();
    bool modified = false;
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        auto arg = mi->substitution.lookup(p);
        if (p->direction == IR::Direction::In ||
            p->direction == IR::Direction::None) {
            auto paramType = typeMap->getType(p, true);
            auto init = convert(arg->expression, paramType);
            CHECK_NULL(init);
            if (init != arg->expression) {
                modified = true;
                convertedArgs->push_back(new IR::Argument(arg->srcInfo, arg->name, init));
                continue;
            }
        }
        convertedArgs->push_back(arg);
    }
    if (modified) {
        LOG2("Converted some function arguments to struct initializers" << convertedArgs);
        return new IR::MethodCallExpression(
            result->srcInfo, result->method, result->typeArguments, convertedArgs);
    }
    return expression;
}


const IR::Node* CreateStructInitializers::postorder(IR::Operation_Relation* expression) {
    auto orig = getOriginal<IR::Operation_Relation>();
    auto ltype = typeMap->getType(orig->left, true);
    auto rtype = typeMap->getType(orig->right, true);
    if (isDefaultInit(expression->right) || isDefaultInit(expression->left)) {
        ::error(ErrorType::ERR_TYPE_ERROR,
                "%1%: ... can only be used for initialization or asignment %2%",
                expression->srcInfo, expression);
        return expression;
    }
    if (ltype->is<IR::Type_StructLike>() && rtype->is<IR::Type_List>())
        expression->right = convert(expression->right, ltype);
    if (rtype->is<IR::Type_StructLike>() && ltype->is<IR::Type_List>())
        expression->left = convert(expression->left, rtype);
    return expression;
}

const IR::Node* CreateStructAssignInitializers::postorder(IR::AssignmentStatement* statement) {
    bool defaultInit = false;
    auto type = typeMap->getType(statement->left);
    auto right = statement->right;
    if ((right->is<IR::ListExpression>() &&
        right->to<IR::ListExpression>()->defaultInitializer) ||
       (right->is<IR::StructExpression>() &&
        right->to<IR::StructExpression>()->defaultInitializer)) {
           defaultInit = true;
    }
    auto init = convert(statement->right, type);
    if (init != statement->right)
        statement->right = init;
    if (defaultInit) {
        if (type->is<IR::Type_Header>()) {
            if (!statement->right->to<IR::StructExpression>()->defaultInitializer) {
                return statement;
            } else {
                auto method = new IR::Member(statement->srcInfo, statement->left,
                                             IR::Type_Header::setInvalid);
                auto mc = new IR::MethodCallExpression(
                    statement->srcInfo, method, new IR::Vector<IR::Argument>());
                auto stat = new IR::MethodCallStatement(mc->srcInfo, mc);
                return stat;
            }
        }
        IR::IndexedVector<IR::StatOrDecl>* retval = new IR::IndexedVector<IR::StatOrDecl>();
        if (type->is<IR::Type_Struct>()) {
            return resolveDefaultInit(statement, type, retval, refMap);
        } else {
            auto defaultTupleInit = resolveDefaultInitTuple(statement->right, type, retval, refMap);
            statement->right = defaultTupleInit;
            retval->push_back(statement);
            auto bst = new IR::BlockStatement(statement->srcInfo, *retval);
            return bst;
        }
    }
    return statement;
}

}  // namespace P4
