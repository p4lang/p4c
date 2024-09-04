/*
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

#include "frontends/p4/methodInstance.h"
#include "syntacticEquivalence.h"
#include "typeChecker.h"

namespace P4 {

const IR::Node *TypeInferenceBase::postorder(const IR::IfStatement *conditional) {
    LOG3("TI Visiting " << dbp(getOriginal()));
    auto type = getType(conditional->condition);
    if (type == nullptr) return conditional;
    if (!type->is<IR::Type_Boolean>())
        typeError("Condition of %1% does not evaluate to a bool but %2%", conditional,
                  type->toString());
    return conditional;
}

const IR::Node *TypeInferenceBase::postorder(const IR::SwitchStatement *stat) {
    LOG3("TI Visiting " << dbp(getOriginal()));
    auto type = getType(stat->expression);
    if (type == nullptr) return stat;

    if (auto ae = type->to<IR::Type_ActionEnum>()) {
        // switch (table.apply(...))
        absl::flat_hash_map<cstring, const IR::Node *, Util::Hash> foundLabels;
        const IR::Node *foundDefault = nullptr;
        for (auto c : stat->cases) {
            if (c->label->is<IR::DefaultExpression>()) {
                if (foundDefault)
                    typeError("%1%: multiple 'default' labels %2%", c->label, foundDefault);
                foundDefault = c->label;
                continue;
            } else if (auto pe = c->label->to<IR::PathExpression>()) {
                cstring label = pe->path->name.name;
                auto [it, inserted] = foundLabels.emplace(label, c->label);
                if (!inserted)
                    typeError("%1%: 'switch' label duplicates %2%", c->label, it->second);
                if (!ae->contains(label))
                    typeError("%1% is not a legal label (action name)", c->label);
            } else {
                typeError("%1%: 'switch' label must be an action name or 'default'", c->label);
            }
        }
    } else {
        // switch (expression)
        Comparison comp;
        comp.left = stat->expression;
        if (isCompileTimeConstant(stat->expression))
            warn(ErrorType::WARN_MISMATCH, "%1%: constant expression in switch", stat->expression);

        auto *sclone = stat->clone();
        bool changed = false;
        for (auto &c : sclone->cases) {
            if (!isCompileTimeConstant(c->label))
                typeError("%1%: must be a compile-time constant", c->label);
            auto lt = getType(c->label);
            if (lt == nullptr) continue;
            if (lt->is<IR::Type_InfInt>() && type->is<IR::Type_Bits>()) {
                c = new IR::SwitchCase(c->srcInfo, new IR::Cast(c->label->srcInfo, type, c->label),
                                       c->statement);
                setType(c->label, type);
                setCompileTimeConstant(c->label);
                changed = true;
                continue;
            } else if (c->label->is<IR::DefaultExpression>()) {
                continue;
            }
            comp.right = c->label;
            bool b = compare(stat, type, lt, &comp);
            if (b && comp.right != c->label) {
                c = new IR::SwitchCase(c->srcInfo, comp.right, c->statement);
                setCompileTimeConstant(c->label);
                changed = true;
            }
        }
        if (changed) stat = sclone;
    }
    return stat;
}

const IR::Node *TypeInferenceBase::postorder(const IR::ReturnStatement *statement) {
    LOG3("TI Visiting " << dbp(getOriginal()));
    auto func = findOrigCtxt<IR::Function>();
    if (func == nullptr) {
        if (statement->expression != nullptr)
            typeError("%1%: return with expression can only be used in a function", statement);
        return statement;
    }

    auto ftype = getType(func);
    if (ftype == nullptr) return statement;

    BUG_CHECK(ftype->is<IR::Type_Method>(), "%1%: expected a method type for function", ftype);
    auto mt = ftype->to<IR::Type_Method>();
    auto returnType = mt->returnType;
    CHECK_NULL(returnType);
    if (returnType->is<IR::Type_Void>()) {
        if (statement->expression != nullptr)
            typeError("%1%: return expression in function with void return", statement);
        return statement;
    }

    if (statement->expression == nullptr) {
        typeError("%1%: return with no expression in a function returning %2%", statement,
                  returnType->toString());
        return statement;
    }

    auto init = assignment(statement, returnType, statement->expression);
    if (init != statement->expression)
        statement = new IR::ReturnStatement(statement->srcInfo, init);
    return statement;
}

const IR::Node *TypeInferenceBase::postorder(const IR::AssignmentStatement *assign) {
    LOG3("TI Visiting " << dbp(getOriginal()));
    auto ltype = getType(assign->left);
    if (ltype == nullptr) return assign;

    if (!isLeftValue(assign->left)) {
        typeError("Expression %1% cannot be the target of an assignment", assign->left);
        LOG2(assign->left);
        return assign;
    }

    auto newInit = assignment(assign, ltype, assign->right);
    if (newInit != assign->right)
        assign = new IR::AssignmentStatement(assign->srcInfo, assign->left, newInit);
    return assign;
}

const IR::Node *TypeInferenceBase::postorder(const IR::ForInStatement *forin) {
    LOG3("TI Visiting " << dbp(getOriginal()));
    auto ltype = getType(forin->ref);
    if (ltype == nullptr) return forin;
    auto ctype = getType(forin->collection);
    if (ctype == nullptr) return forin;

    if (!isLeftValue(forin->ref)) {
        typeError("Expression %1% cannot be the target of an assignment", forin->ref);
        LOG2(forin->ref);
        return forin;
    }
    if (auto range = forin->collection->to<IR::Range>()) {
        auto rclone = range->clone();
        rclone->left = assignment(forin, ltype, rclone->left);
        rclone->right = assignment(forin, ltype, rclone->right);
        if (*range != *rclone)
            forin = new IR::ForInStatement(forin->srcInfo, forin->annotations, forin->decl, rclone,
                                           forin->body);
        else
            delete rclone;
    } else if (auto *stack = ctype->to<IR::Type_Stack>()) {
        if (!canCastBetween(stack->elementType, ltype))
            typeError("%1% does not match header stack type %2%", forin->ref, ctype);
    } else if (auto *list = ctype->to<IR::Type_P4List>()) {
        if (!canCastBetween(list->elementType, ltype))
            typeError("%1% does not match %2% element type", forin->ref, ctype);
    } else {
        error(ErrorType::ERR_UNSUPPORTED,
              "%1%Typechecking does not support iteration over this collection of type %2%",
              forin->collection->srcInfo, ctype);
    }
    return forin;
}

const IR::Node *TypeInferenceBase::postorder(const IR::ActionListElement *elem) {
    if (done()) return elem;
    auto type = getType(elem->expression);
    if (type == nullptr) return elem;

    setType(elem, type);
    setType(getOriginal(), type);
    return elem;
}

const IR::Node *TypeInferenceBase::postorder(const IR::SelectCase *sc) {
    auto type = getType(sc->state);
    if (type != nullptr && type != IR::Type_State::get()) typeError("%1% must be state", sc);
    return sc;
}

const IR::Node *TypeInferenceBase::postorder(const IR::KeyElement *elem) {
    auto ktype = getType(elem->expression);
    if (ktype == nullptr) return elem;
    while (ktype->is<IR::Type_Newtype>()) ktype = getTypeType(ktype->to<IR::Type_Newtype>()->type);
    if (!ktype->is<IR::Type_Bits>() && !ktype->is<IR::Type_SerEnum>() &&
        !ktype->is<IR::Type_Error>() && !ktype->is<IR::Type_Boolean>() &&
        !ktype->is<IR::Type_Enum>())
        typeError("Key %1% field type must be a scalar type; it cannot be %2%", elem->expression,
                  ktype->toString());
    auto type = getType(elem->matchType);
    if (type != nullptr && type != IR::Type_MatchKind::get())
        typeError("%1% must be a %2% value", elem->matchType,
                  IR::Type_MatchKind::get()->toString());
    if (isCompileTimeConstant(elem->expression) && !readOnly)
        warn(ErrorType::WARN_IGNORE_PROPERTY, "%1%: constant key element", elem);
    return elem;
}

const IR::Node *TypeInferenceBase::postorder(const IR::ActionList *al) {
    LOG3("TI Visited " << dbp(al));
    BUG_CHECK(currentActionList == nullptr, "%1%: nested action list?", al);
    currentActionList = al;
    return al;
}

const IR::ActionListElement *TypeInferenceBase::validateActionInitializer(
    const IR::Expression *actionCall) {
    // We cannot retrieve the action list from the table, because the
    // table has not been modified yet.  We want the latest version of
    // the action list, as it has been already typechecked.
    auto al = currentActionList;
    if (al == nullptr) {
        auto table = findContext<IR::P4Table>();
        BUG_CHECK(table, "%1%: not within a table", actionCall);
        typeError("%1% has no action list, so it cannot invoke '%2%'", table, actionCall);
        return nullptr;
    }

    auto call = actionCall->to<IR::MethodCallExpression>();
    if (call == nullptr) {
        typeError("%1%: expected an action call", actionCall);
        return nullptr;
    }
    auto method = call->method;
    if (!method->is<IR::PathExpression>()) BUG("%1%: unexpected expression", method);
    auto pe = method->to<IR::PathExpression>();
    auto decl = getDeclaration(pe->path, !errorOnNullDecls);
    if (errorOnNullDecls && decl == nullptr) {
        typeError("%1%: Cannot resolve declaration", pe);
        return nullptr;
    }

    auto ale = al->actionList.getDeclaration(decl->getName());
    if (ale == nullptr) {
        typeError("%1% not present in action list", call);
        return nullptr;
    }

    BUG_CHECK(ale->is<IR::ActionListElement>(), "%1%: expected an ActionListElement", ale);
    auto elem = ale->to<IR::ActionListElement>();
    auto entrypath = elem->getPath();
    auto entrydecl = getDeclaration(entrypath, true);
    if (entrydecl != decl) {
        typeError("%1% and %2% refer to different actions", actionCall, elem);
        return nullptr;
    }

    // Check that the data-plane parameters
    // match the data-plane parameters for the same action in
    // the actions list.
    auto actionListCall = elem->expression->to<IR::MethodCallExpression>();
    CHECK_NULL(actionListCall);
    auto type = typeMap->getType(actionListCall->method);
    if (type == nullptr) {
        typeError("%1%: action invocation should be after the `actions` list", actionCall);
        return nullptr;
    }

    if (actionListCall->arguments->size() > call->arguments->size()) {
        typeError("%1%: not enough arguments", call);
        return nullptr;
    }

    SameExpression se(this, typeMap);
    auto callInstance = MethodInstance::resolve(call, this, typeMap, getChildContext(), true);
    auto listInstance =
        MethodInstance::resolve(actionListCall, this, typeMap, getChildContext(), true);

    for (auto param : *listInstance->substitution.getParametersInArgumentOrder()) {
        auto aa = listInstance->substitution.lookup(param);
        auto da = callInstance->substitution.lookup(param);
        if (da == nullptr) {
            typeError("%1%: parameter should be assigned in call %2%", param, call);
            return nullptr;
        }
        bool same = se.sameExpression(aa->expression, da->expression);
        if (!same) {
            typeError("%1%: argument does not match declaration in actions list: %2%", da, aa);
            return nullptr;
        }
    }

    for (auto param : *callInstance->substitution.getParametersInOrder()) {
        auto da = callInstance->substitution.lookup(param);
        if (da == nullptr) {
            typeError("%1%: parameter should be assigned in call %2%", param, call);
            return nullptr;
        }
    }

    return elem;
}

const IR::Node *TypeInferenceBase::postorder(const IR::Property *prop) {
    // Handle the default_action
    if (prop->name != IR::TableProperties::defaultActionPropertyName) return prop;

    auto pv = prop->value->to<IR::ExpressionValue>();
    if (pv == nullptr) {
        typeError("%1% table property should be an action", prop);
    } else {
        auto type = getType(pv->expression);
        if (type == nullptr) return prop;
        if (!type->is<IR::Type_Action>()) {
            typeError("%1% table property should be an action", prop);
            return prop;
        }
        auto at = type->to<IR::Type_Action>();
        if (at->parameters->size() != 0) {
            typeError("%1%: parameter %2% does not have a corresponding argument", prop->value,
                      at->parameters->parameters.at(0));
            return prop;
        }

        // Check that the default action appears in the list of actions.
        BUG_CHECK(prop->value->is<IR::ExpressionValue>(), "%1% not an expression", prop);
        auto def = prop->value->to<IR::ExpressionValue>()->expression;
        auto ale = validateActionInitializer(def);
        if (ale != nullptr) {
            auto anno = ale->getAnnotation(IR::Annotation::tableOnlyAnnotation);
            if (anno != nullptr) {
                typeError("%1%: Action marked with %2% used as default action", prop,
                          IR::Annotation::tableOnlyAnnotation);
                return prop;
            }
        }
    }

    return prop;
}

}  // namespace P4
