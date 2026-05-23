/* Copyright 2022 Syrmia Networks

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

#include "decl_copyprop.h"

namespace P4 {

/// Convert an expression into a string that uniqely identifies the variable referenced.
/// @return null cstring if not a reference to a variable.
cstring DoDeclarationCopyPropagation::getName(const IR::Expression* expr) {
    if (auto p = expr->to<IR::PathExpression>())
        return p->path->name;
    if (auto m = expr->to<IR::Member>()) {
        if (auto base = getName(m->expr))
            return base + "." + m->member;
    } else if (auto a = expr->to<IR::ArrayIndex>()) {
        if (auto k = a->right->to<IR::Constant>()) {
            if (auto base = getName(a->left))
                return base + "[" + std::to_string(k->asInt()) + "]";
        }
    }
    return cstring();
}

/// Checks if the initializer has a constant.
/// If it is a cast, checks if the innermost expression is a constant.
/// It checks both cases because it needs to retain the casts.
/// @return true if a constant is found.
bool DoDeclarationCopyPropagation::checkConst(const IR::Expression* expr) {
    if (!expr) {
        return false;
    }
    if (expr->is<IR::Constant>()) {
        return true;
    }
    if (expr->is<IR::BoolLiteral>()) {
        return true;
    }
    if (auto list = expr->to<IR::ListExpression>()) {
        for (auto el : list->components) {
            if (!checkConst(el)) {
                return false;
            }
        }
        return true;
    } else if (auto strct = expr->to<IR::StructExpression>()) {
        for (auto ne : strct->components) {
            if (!checkConst(ne->expression)) {
                return false;
            }
        }
        return true;
    } else if (auto cast = expr->to<IR::Cast>()) {
        return checkConst(cast->expr);
    }

    return false;
}

/// Finds the declared value of an expression.
/// If it is a path expression it gets the declaration from the reference map, and if
/// the declaration has an initializer that is a constant, and sets it as the return value.
/// If it is a member or an array it recursively finds the outermost expression and gets the
/// declaration. Then searches the declaration for the member part, and if it finds that it
/// is a constant, sets it as the return value.
/// If the return value is set it adds it to the map so that it can be reused without searching.
/// @return constant value declared to the expression @param expr
const IR::Expression* DoDeclarationCopyPropagation::getValue(const IR::Expression* expr) {
    const IR::Expression* retVal = nullptr;
    if (auto pathExpr = expr->to<IR::PathExpression>()) {
        auto decl = refMap->getDeclaration(pathExpr->path);
        if (!decl) {
            return nullptr;
        }
        auto declVar = decl->to<IR::Declaration_Variable>();
        if (!declVar || !declVar->initializer) {
            return nullptr;
        }
        if (checkOnly) {
            setCompileTimeConstant(getOriginal<IR::Expression>());
            return nullptr;
        }
        auto init = declVar->initializer;
        if ((getName(expr) == getName(getOriginal<IR::Expression>()) && checkConst(init))) {
            retVal = init;
        } else if (init->is<IR::StructExpression>() || init->is<IR::ListExpression>()) {
            // returns the initializer up the recursion so it can be processed
            return init;
        } else {
            return nullptr;
        }
    } else if (auto member = expr->to<IR::Member>()) {
        auto memberExpr = member->expr;
        auto memberName = member->member.name;
        auto currExpr = getValue(memberExpr);
        if (!currExpr) {
            return nullptr;
        }
        if (auto structExpr = currExpr->to<IR::StructExpression>()) {
            auto ne = structExpr->components.getDeclaration<IR::NamedExpression>(memberName);
            if (!ne) {
                return nullptr;
            }
            if ((getName(expr) == getName(getOriginal<IR::Expression>()) &&
                 checkConst(ne->expression))) {
                retVal = ne->expression;
            } else {
                return ne->expression;
            }
        }
    } else if (auto array = expr->to<IR::ArrayIndex>()) {
        auto arrayExpr = array->left;
        auto arrayInd = array->right->to<IR::Constant>();
        auto currExpr = getValue(arrayExpr);
        if (!currExpr || !arrayInd) {
            return nullptr;
        }
        auto index = (size_t)arrayInd->value;
        if (auto listExpr = currExpr->to<IR::ListExpression>()) {
            if (listExpr->components.size() <= index) {
                return nullptr;
            }
            auto el = listExpr->components.at(index);
            if (!el) {
                return nullptr;
            }
            if ((getName(expr) == getName(getOriginal<IR::Expression>()) && checkConst(el))) {
                retVal = el;
            } else {
                return el;
            }
        }
    }

    if (retVal) {
        varMap[getName(expr)] = retVal;
        return retVal;
    }

    return nullptr;
}

/// Checks if the value of the expression is already used and in the map, and if it is, returns
/// the value. If the value is not in the map it searches for it in the variable declaration.
/// @return constant value declared to the expression @param expr
const IR::Expression* DoDeclarationCopyPropagation::findValue(const IR::Expression* expr) {
    if (auto val = ::get(varMap, getName(expr))) {
        LOG5("Propagating value: " << val << " for variable: " << getName(expr));
        return val;
    }

    auto val = getValue(expr);
    if (!checkConst(val)) {
        // return nullptr if getValue accidentally returns a non constant value
        return nullptr;
    }

    LOG5("Propagating value: " << val << " for variable: " << getName(expr));
    return val;
}

/// Start propagating at the beggining of the control
const IR::Node* DoDeclarationCopyPropagation::preorder(IR::P4Control* control) {
    LOG2("DeclarationCopyPropagation working on control: " << control->getName());
    working = true;
    return control;
}

/// Do not visit actions
const IR::Node* DoDeclarationCopyPropagation::preorder(IR::P4Action* action) {
    prune();
    return action;
}

/// Do not visit tables
const IR::Node* DoDeclarationCopyPropagation::preorder(IR::P4Table* table) {
    prune();
    return table;
}

/// Stop propagating when the apply block is reached, only control locals are propagated
const IR::Node* DoDeclarationCopyPropagation::preorder(IR::BlockStatement* block) {
    auto control = findContext<IR::P4Control>();
    if (!control || !(*control->body == *block)) {
        return block;
    }
    working = false;
    prune();
    return block;
}

const IR::Node* DoDeclarationCopyPropagation::preorder(IR::MethodCallExpression* method) {
    if (!working || checkOnly) {
        return method;
    }

    auto instance = MethodInstance::resolve(method, refMap, typeMap, true);
    auto paramIt = instance->getActualParameters()->begin();
    auto arguments = new IR::Vector<IR::Argument>();
    for (auto arg : *method->arguments) {
        auto param = *paramIt++;
        if (param->direction == IR::Direction::Out || param->direction == IR::Direction::InOut) {
            arguments->push_back(arg);
            continue;
        }
        if (auto newArg = findValue(arg->expression)) {
            arguments->push_back(new IR::Argument(arg->srcInfo, arg->name, newArg));
        } else {
            arguments->push_back(arg);
        }
    }
    if (!method->arguments->equiv(*arguments)) {
        method->arguments = arguments;
    }
    prune();

    return method;
}

/// Do not visit functions
const IR::Node* DoDeclarationCopyPropagation::preorder(IR::Function* func) {
    prune();
    return func;
}

/// Clear the map at the end of every control
const IR::Node* DoDeclarationCopyPropagation::postorder(IR::P4Control* control) {
    LOG2("DeclarationCopyPropagation finished working on control: " << control->getName());
    varMap.clear();
    working = false;
    return control;
}

/// If we are in the local part of the control, find the value of the member and
/// if it exists return it at the place of the expression
const IR::Node* DoDeclarationCopyPropagation::preorder(IR::Member* member) {
    if (!working) {
        return member;
    }

    if (getName(member)) {
        prune();
        if (auto val = findValue(member)) {
            return val;
        }
    }

    return member;
}

/// If we are in the local part of the control, find the value of the variable and
/// if it exists return it at the place of the expression
const IR::Node* DoDeclarationCopyPropagation::postorder(IR::PathExpression* path) {
    if (!working) {
        return path;
    }

    if (auto val = findValue(path)) {
        return val;
    }

    return path;
}

/// If we are in the local part of the control, find the value of the array element
/// and if it exists return it at the place of the expression
const IR::Node* DoDeclarationCopyPropagation::preorder(IR::ArrayIndex* array) {
    if (!working) {
        return array;
    }

    if (getName(array)) {
        prune();
        if (auto val = findValue(array)) {
            return val;
        }
    }

    return array;
}


} /* namespace p4 */
