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

/// Given a type and srcInfo synthesize default value for that type
/// Default values are as proposed in p4-spec
/// For types that dont have defined values in p4-spec give error
const IR::Expression* CreateStructInitializers::defaultValue(const IR::Type* type,
                                                            const Util::SourceInfo srcInfo) {
    IR::Expression* expr = nullptr;
    if (type->is<IR::Type_Bits>()) {
        expr = new IR::Constant(srcInfo, type, 0);
    } else if (type->is<IR::Type_Boolean>()) {
        expr = new IR::BoolLiteral(srcInfo, type, false);
    } else if (type->is<IR::Type_Error>()) {
        auto typeName = new IR::Type_Name(srcInfo, new IR::Path(IR::ID(srcInfo, "error")));
        expr = new IR::Member(srcInfo, new IR::TypeNameExpression(srcInfo, typeName), "NoError");
    } else if (auto t = type->to<IR::Type_Enum>()) {
        BUG_CHECK(t->members.size(), "unknown default value for enum with no members");
        auto tne = new IR::TypeNameExpression(srcInfo,
                                               new IR::Type_Name(t->name));
        expr = new IR::Member(srcInfo, tne, t->members.at(0)->name);
    } else if (auto t = type->to<IR::Type_SerEnum>()) {
        auto zero = new IR::Constant(t->type, 0);
        expr = new IR::Cast(t->getP4Type(), zero);
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
    CHECK_NULL(expr);
    return expr;
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
        CHECK_NULL(hu);
        BUG_CHECK(hu->fields.size(), "Expected union %1% to have at least one field", hu);
        auto hdr = hu->fields.at(0);
        auto left = new IR::Member(exp, hdr->name);
        auto method = new IR::Member(srcInfo, left,
                                     IR::Type_Header::setInvalid);
        auto mc = new IR::MethodCallExpression(srcInfo, method, new IR::Vector<IR::Argument>());
        return new IR::MethodCallStatement(mc->srcInfo, mc);
    }
}

/// @param rexpr is a struct expression or list expression
/// used to assign a value to a struct or a tuple
/// @param ltype respectively, ltype is destination type
/// @param retval is a vector with expanded initializers for rexpr
const IR::Expression* resolveDefaultInitTuple(const IR::Expression* rexpr,
                                              const IR::Type* ltype,
                                              IR::IndexedVector<IR::StatOrDecl>* retval,
                                              ReferenceMap* refMap, bool isDeclaration = false) {
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
                if (!isDeclaration) {
                    auto stat = generateSetInvalid(type, rexpr->srcInfo, pe);
                    retval->push_back(stat);
                }
            } else if (type->is<IR::Type_BaseList>() || type->is<IR::Type_Struct>()) {
                vec->push_back(resolveDefaultInitTuple(expr, type, retval, refMap, isDeclaration));
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
                if (!isDeclaration) {
                    auto stat = generateSetInvalid(f->type, rexpr->srcInfo, pe);
                    retval->push_back(stat);
                }
            } else if (f->type->is<IR::Type_BaseList>() || f->type->is<IR::Type_Struct>()) {
                ne = new IR::NamedExpression(rexpr->srcInfo, f->name,
                     resolveDefaultInitTuple(ne->expression, f->type,
                                            retval, refMap, isDeclaration));
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
                                        ReferenceMap* refMap, bool isDeclaration = false) {
    auto strct = ltype->to<IR::Type_StructLike>();
    if (auto si = statement->right->to<IR::StructExpression>()) {
        for (auto f : strct->fields) {
            auto right = si->components.getDeclaration<IR::NamedExpression>(f->name);
            auto left = new IR::Member(statement->left, f->name);
            if ((f->type->is<IR::Type_Header>() && right->expression->hasDefaultInitializer()) ||
                 f->type->is<IR::Type_HeaderUnion>()) {
                if (!isDeclaration) {
                    auto stat = generateSetInvalid(f->type, statement->srcInfo, left);
                    retval->push_back(stat);
                }
                continue;
            } else if (f->type->is<IR::Type_Struct>()) {
                resolveDefaultInit(new IR::AssignmentStatement(statement->srcInfo, left,
                                   right->expression), f->type, retval, refMap, isDeclaration);
                continue;
            } else if (f->type->is<IR::Type_BaseList>()) {
                auto expr = resolveDefaultInitTuple(right->expression, f->type,
                                                    retval, refMap, isDeclaration);
                retval->push_back(new IR::AssignmentStatement(
                                  statement->srcInfo, left, expr));
                continue;
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
    bool hasDefault = false;
    CHECK_NULL(type);
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
                hasDefault |= conv->hasDefaultInitializer();
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
                for (; index < st->fields.size(); index++) {
                    auto f = st->fields.at(index);
                    if (f->type->is<IR::Type_Varbits>() || f->type->is<IR::Type_Stack>()) {
                        ::error(ErrorType::ERR_UNSUPPORTED,
                        "Structs, headers and tuples "
                        "containing fields %1% cannot be initialized "
                        "using ...: %2% = {...}.",
                        f->type, le);
                    }
                    if (!hdrInvalid) {
                        expr = CreateStructInitializers::defaultValue(f->type, expression->srcInfo);
                        hasDefault |= expr->hasDefaultInitializer();
                        auto ne = new IR::NamedExpression(expression->srcInfo, f->name, expr);
                        si->push_back(ne);
                    }
                }
            }
            auto type = st->getP4Type()->to<IR::Type_Name>();
            auto result = new IR::StructExpression(
                expression->srcInfo, type, type, *si, hdrInvalid | hasDefault);
            return result;
        } else if (auto sli = expression->to<IR::StructExpression>()) {
            if (!sli->defaultInitializer) {
                for (auto f : st->fields) {
                    auto ne = sli->components.getDeclaration<IR::NamedExpression>(f->name.name);
                    BUG_CHECK(ne != nullptr, "%1%: no initializer for %2%", expression, f);
                    auto convNe = convert(ne->expression, f->type);
                    if (convNe != ne->expression)
                        modified = true;
                    hasDefault |= convNe->hasDefaultInitializer();
                    ne = new IR::NamedExpression(ne->srcInfo, f->name, convNe);
                    si->push_back(ne);
                }
            } else {
                modified = true;
                for (auto f : st->fields) {
                    if (f->type->is<IR::Type_Varbits>() || f->type->is<IR::Type_Stack>()) {
                        ::error(ErrorType::ERR_UNSUPPORTED,
                        "Structs, headers and tuples "
                        "containing fields %1% cannot be initialized "
                        "using ...: %2% = {...}.",
                        f->type, sli);
                    }
                    auto ne = sli->components.getDeclaration<IR::NamedExpression>(f->name.name);
                    if (ne == nullptr) {
                        const IR::Expression* expr;
                        expr = CreateStructInitializers::defaultValue(f->type, expression->srcInfo);
                        hasDefault |= expr->hasDefaultInitializer();
                        auto ne = new IR::NamedExpression(expression->srcInfo, f->name, expr);
                        si->push_back(ne);
                    } else {
                        auto convNe = convert(ne->expression, f->type);
                        hasDefault |= convNe->hasDefaultInitializer();
                        ne = new IR::NamedExpression(ne->srcInfo, f->name, convNe);
                        si->push_back(ne);
                    }
                }
                if (sli->defaultInitializer && sli->components.size() == 0 && isHdr)
                    hdrInvalid = true;
            }
            if (modified || sli->type->is<IR::Type_UnknownStruct>()) {
                auto type = st->getP4Type()->to<IR::Type_Name>();
                auto result = new IR::StructExpression(
                    expression->srcInfo, type, type, *si, hdrInvalid | hasDefault);
                return result;
            }
        } else if (auto mux = expression->to<IR::Mux>()) {
            auto e1 = convert(mux->e1, type);
            auto e2 = convert(mux->e2, type);
            return new IR::Mux(mux->e0, e1, e2);
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
            hasDefault |= conv->hasDefaultInitializer();
            vec->push_back(conv);
            modified |= (conv != expr);
        }
        if (le->defaultInitializer) {
            const IR::Expression* expr;
            modified = true;
            for ( ; index < tup->components.size(); index++) {
                auto f = tup->components.at(index);
                if (f->is<IR::Type_Varbits>() || f->is<IR::Type_Stack>()) {
                ::error(ErrorType::ERR_UNSUPPORTED,
                        "Structs, headers and tuples "
                        "containing fields %1% cannot be initialized "
                        "using ...: %2% = {...}.",
                        type, le);
                }
                expr = CreateStructInitializers::defaultValue(f, expression->srcInfo);
                hasDefault |= expr->hasDefaultInitializer();
                vec->push_back(expr);
            }
        }
        if (modified) {
            auto result = new IR::ListExpression(expression->srcInfo, *vec, hasDefault);
            return result;
        }
    }
    return expression;
}
const IR::Node* CreateStructInitializers::postorder(IR::ReturnStatement* statement) {
    if (statement->expression == nullptr)
        return statement;
    auto func = findOrigCtxt<IR::Function>();
    if (func == nullptr)
        return statement;

    auto ftype = typeMap->getType(func);
    BUG_CHECK(ftype->is<IR::Type_Method>(), "%1%: expected a method type for function", ftype);
    auto mt = ftype->to<IR::Type_Method>();
    auto returnType = mt->returnType;
    CHECK_NULL(returnType);
    // MoveDefaultInitializers pass should move default initialized return statements
    BUG_CHECK(!statement->expression->hasDefaultInitializer(), "Unexpected default initialization"
                        " %1% as a return statement in function %2%", statement->expression, func);
    statement->expression = convert(statement->expression, returnType);
    return statement;
}

const IR::Node* CreateStructInitializers::postorder(IR::Declaration_Variable* decl) {
    if (decl->initializer == nullptr)
        return decl;
    bool defaultInit = false;
    auto type = typeMap->getTypeType(decl->type, true);
    auto init = convert(decl->initializer, type);
    if (init != decl->initializer)
        decl->initializer = init;

    if (decl->initializer->hasDefaultInitializer()) {
        defaultInit = true;
    }
    if (defaultInit) {
        auto left = new IR::PathExpression(decl->name);
        auto statement = new IR::AssignmentStatement(decl->srcInfo, left, decl->initializer);
        if (type->is<IR::Type_Header>()) {
            decl->initializer = nullptr;
            return decl;
        }
        IR::IndexedVector<IR::StatOrDecl>* retval = new IR::IndexedVector<IR::StatOrDecl>();
        if (type->is<IR::Type_Struct>()) {
            decl->initializer = nullptr;
            retval->push_back(decl);
            resolveDefaultInit(statement, decl->type, retval, refMap, true);
            return retval;
        } else if (type->is<IR::Type_BaseList>()) {
            decl->initializer = nullptr;
            retval->push_back(decl);
            auto defaultTupleInit = resolveDefaultInitTuple(statement->right, type,
                                                            retval, refMap, true);
            statement->right = defaultTupleInit;
            retval->push_back(statement);
            return retval;
        } else {
            BUG("%1% should be Type_Header, Type_Struct or Type_BaseList, not type %2%",
                statement->left, type);
        }
    }
    return decl;
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
            if (paramType == nullptr)
                // on error
                continue;
            BUG_CHECK(!arg->expression->hasDefaultInitializer(), "Unexpected default initialization"
                                " %1% as argument in function %2%", arg->expression, expression);
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
    // MoveDefaultInitializers pass should move all default initialized operands
    BUG_CHECK(!expression->left->hasDefaultInitializer(), "Unexpected default initialization %1%"
                                    " in operation relation %2%", expression->left, expression);
    BUG_CHECK(!expression->right->hasDefaultInitializer(), "Unexpected default initialization %1%"
                                    " in operation relation %2%", expression->right, expression);
    if (ltype->is<IR::Type_StructLike>() && rtype->is<IR::Type_List>())
        expression->right = convert(expression->right, ltype);
    if (rtype->is<IR::Type_StructLike>() && ltype->is<IR::Type_List>())
        expression->left = convert(expression->left, rtype);
    return expression;
}

const IR::Node* CreateStructInitializers::postorder(IR::AssignmentStatement* statement) {
    bool defaultInit = false;
    auto type = typeMap->getType(getOriginal<IR::AssignmentStatement>()->left);
    auto init = convert(statement->right, type);
    if (init != statement->right)
        statement->right = init;
    if (statement->right->hasDefaultInitializer()) {
        defaultInit = true;
    }
    if (defaultInit) {
        if (type->is<IR::Type_Header>()) {
            auto method = new IR::Member(statement->srcInfo, statement->left,
                                            IR::Type_Header::setInvalid);
            auto mc = new IR::MethodCallExpression(
                statement->srcInfo, method, new IR::Vector<IR::Argument>());
            auto stat = new IR::MethodCallStatement(mc->srcInfo, mc);
            return stat;
        }
        IR::IndexedVector<IR::StatOrDecl>* retval = new IR::IndexedVector<IR::StatOrDecl>();
        if (type->is<IR::Type_Struct>()) {
            return resolveDefaultInit(statement, type, retval, refMap);
        } else if (type->is<IR::Type_BaseList>()) {
            auto defaultTupleInit = resolveDefaultInitTuple(statement->right, type, retval, refMap);
            statement->right = defaultTupleInit;
            retval->push_back(statement);
            auto bst = new IR::BlockStatement(statement->srcInfo, *retval);
            return bst;
        } else {
            BUG("%1% should be Type_Header, Type_Struct or Type_BaseList, not type %2%",
                statement->left, type);
        }
    }
    return statement;
}


const IR::Node* MoveDefaultInitialization::postorder(IR::Operation_Relation* expression) {
    auto orig = getOriginal<IR::Operation_Relation>();
    auto ltype = typeMap->getType(orig->left, true);
    auto rtype = typeMap->getType(orig->right, true);
    if (expression->left->hasDefaultInitializer() && expression->right->hasDefaultInitializer()) {
        ::error(ErrorType::ERR_TYPE_ERROR,
                "Operation relation %1% cannot be used in a case"
                " where both operands are default initialized",
                expression);
        return expression;
    }

    if (expression->left->hasDefaultInitializer()) {
        // create a temporary variable for default initialization of left operand expression
        auto name = refMap->newName("leftTmp");
        auto leftDecl = new IR::Declaration_Variable(expression->srcInfo, name,
                                                    rtype, expression->left);
        auto leftPathExpr = new IR::PathExpression(leftDecl->srcInfo,
                                                    new IR::Path(leftDecl->name));
        toMove->push_back(leftDecl);
        expression->left = leftPathExpr;
    } else if (expression->right->hasDefaultInitializer()) {
        // create a temporary variable for default initialization of right operand expression
        auto name = refMap->newName("rightTmp");
        auto rightDecl = new IR::Declaration_Variable(expression->srcInfo, name,
                                                    ltype, expression->right);
        auto rightPathExpr = new IR::PathExpression(rightDecl->srcInfo,
                                                    new IR::Path(rightDecl->name));
        toMove->push_back(rightDecl);
        expression->right = rightPathExpr;
    }
    return expression;
}

const IR::Node* MoveDefaultInitialization::postorder(IR::MethodCallExpression* expression) {
    auto mi = MethodInstance::resolve(expression, refMap, typeMap);
    auto result = expression;
    auto convertedArgs = new IR::Vector<IR::Argument>();
    bool modified = false;
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        auto arg = mi->substitution.lookup(p);
        if (p->direction == IR::Direction::In ||
            p->direction == IR::Direction::None) {
            auto paramType = typeMap->getType(p, true);
            if (paramType == nullptr)
                // on error
                continue;
            if (arg->expression->hasDefaultInitializer()) {
                // create a temporary variable for default initialization of an argument expression
                auto name = refMap->newName("argTmp");
                auto argDecl = new IR::Declaration_Variable(arg->expression->srcInfo, name,
                                                           paramType, arg->expression);
                auto argPathExpr = new IR::PathExpression(argDecl->srcInfo,
                                                          new IR::Path(argDecl->name));
                toMove->push_back(argDecl);
                convertedArgs->push_back(new IR::Argument(arg->srcInfo, arg->name, argPathExpr));
                modified = true;
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

const IR::Node* MoveDefaultInitialization::postorder(IR::ReturnStatement* statement) {
    if (statement->expression == nullptr)
        return statement;
    if (!statement->expression->hasDefaultInitializer())
        return statement;

    auto func = findOrigCtxt<IR::Function>();
    if (func == nullptr)
        return statement;

    auto ftype = typeMap->getType(func);
    BUG_CHECK(ftype->is<IR::Type_Method>(), "%1%: expected a method type for function", ftype);
    auto mt = ftype->to<IR::Type_Method>();
    auto returnType = mt->returnType;
    CHECK_NULL(returnType);
    // create a temporary variable for default initialization of return expression
    auto name = refMap->newName("returnTmp");
    auto returnDecl = new IR::Declaration_Variable(statement->srcInfo, name,
                                                  returnType, statement->expression);
    auto returnPathExpr = new IR::PathExpression(returnDecl->srcInfo,
                                                 new IR::Path(returnDecl->name));
    auto returnStmt = new IR::ReturnStatement(statement->srcInfo, returnPathExpr);
    auto replaced = new IR::IndexedVector<IR::StatOrDecl>();
    replaced->push_back(returnDecl);
    replaced->push_back(returnStmt);

    return replaced;
}

const IR::Node* MoveDefaultInitialization::postorder(IR::Function* function) {
    if (toMove->empty())
        return function;
    toMove->append(function->body->components);
    auto newBody = new IR::BlockStatement(function->body->annotations, *toMove);
    function->body = newBody;
    toMove = new IR::IndexedVector<IR::StatOrDecl>();
    return function;
}

const IR::Node* MoveDefaultInitialization::postorder(IR::ParserState* state) {
    if (toMove->empty())
        return state;
    toMove->append(state->components);
    state->components = *toMove;
    toMove = new IR::IndexedVector<IR::StatOrDecl>();
    return state;
}

const IR::Node* MoveDefaultInitialization::postorder(IR::P4Control* control) {
    if (toMove->empty())
        return control;
    toMove->append(control->body->components);
    auto newBody = new IR::BlockStatement(control->body->annotations, *toMove);
    control->body = newBody;
    toMove = new IR::IndexedVector<IR::StatOrDecl>();
    return control;
}

}  // namespace P4
