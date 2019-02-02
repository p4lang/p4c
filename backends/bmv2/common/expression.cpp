/*Copyright 2013-present Barefoot Networks, Inc.

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

#include "expression.h"
#include "helpers.h"

namespace BMV2 {

class ArithmeticFixup;

const IR::Expression* ArithmeticFixup::fix(const IR::Expression* expr, const IR::Type_Bits* type) {
    unsigned width = type->size;
    if (!type->isSigned) {
        auto mask = new IR::Constant(type, Util::mask(width), 16);
        typeMap->setType(mask, type);
        auto result = new IR::BAnd(expr->srcInfo, expr, mask);
        typeMap->setType(result, type);
        return result;
    } else {
        auto result = new IR::IntMod(expr->srcInfo, expr, width);
        typeMap->setType(result, type);
        return result;
    }
    return expr;
}

const IR::Node* ArithmeticFixup::updateType(const IR::Expression* expression) {
    if (*expression != *getOriginal()) {
        auto type = typeMap->getType(getOriginal(), true);
        typeMap->setType(expression, type);
    }
    return expression;
}

const IR::Node* ArithmeticFixup::postorder(IR::Expression* expression) {
    return updateType(expression);
}

const IR::Node* ArithmeticFixup::postorder(IR::Operation_Binary* expression) {
    auto type = typeMap->getType(getOriginal(), true);
    if (expression->is<IR::BAnd>() || expression->is<IR::BOr>() ||
        expression->is<IR::BXor>() ||
        expression->is<IR::AddSat>() || expression->is<IR::SubSat>())
        // no need to clamp these
        return updateType(expression);
    if (type->is<IR::Type_Bits>())
        return fix(expression, type->to<IR::Type_Bits>());
    return updateType(expression);
}

const IR::Node* ArithmeticFixup::postorder(IR::Neg* expression) {
    auto type = typeMap->getType(getOriginal(), true);
    if (type->is<IR::Type_Bits>())
        return fix(expression, type->to<IR::Type_Bits>());
    return updateType(expression);
}

const IR::Node* ArithmeticFixup::postorder(IR::Cmpl* expression) {
    auto type = typeMap->getType(getOriginal(), true);
    if (type->is<IR::Type_Bits>())
        return fix(expression, type->to<IR::Type_Bits>());
    return updateType(expression);
}

const IR::Node* ArithmeticFixup::postorder(IR::Cast* expression) {
    auto type = typeMap->getType(getOriginal(), true);
    if (type->is<IR::Type_Bits>())
        return fix(expression, type->to<IR::Type_Bits>());
    return updateType(expression);
}

void ExpressionConverter::mapExpression(const IR::Expression* expression, Util::IJson* json) {
    map.emplace(expression, json);
    LOG3("Mapping " << dbp(expression) << " to " << json->toString());
}

Util::IJson* ExpressionConverter::get(const IR::Expression* expression) const {
    auto result = ::get(map, expression);
    if (result == nullptr) {
        LOG3("Looking up " << expression);
        for (auto it : map) {
            LOG3(" " << it.first << " " << it.second);
        }
    }
    BUG_CHECK(result, "%1%: could not convert to Json", expression);
    return result;
}

void ExpressionConverter::postorder(const IR::BoolLiteral* expression)  {
    auto result = new Util::JsonObject();
    result->emplace("type", "bool");
    result->emplace("value", expression->value);
    mapExpression(expression, result);
}

void ExpressionConverter::postorder(const IR::MethodCallExpression* expression)  {
    auto instance = P4::MethodInstance::resolve(expression, refMap, typeMap);
    if (instance->is<P4::ExternMethod>()) {
        auto em = instance->to<P4::ExternMethod>();
        if (em->originalExternType->name == corelib.packetIn.name &&
            em->method->name == corelib.packetIn.lookahead.name) {
            BUG_CHECK(expression->typeArguments->size() == 1,
                      "Expected 1 type parameter for %1%", em->method);
            auto targ = expression->typeArguments->at(0);
            auto typearg = typeMap->getTypeType(targ, true);
            int width = typearg->width_bits();
            BUG_CHECK(width > 0, "%1%: unknown width", targ);
            auto j = new Util::JsonObject();
            j->emplace("type", "lookahead");
            auto v = mkArrayField(j, "value");
            v->append(0);
            v->append(width);
            mapExpression(expression, j);
            return;
        }
    } else if (instance->is<P4::BuiltInMethod>()) {
        auto bim = instance->to<P4::BuiltInMethod>();
        if (bim->name == IR::Type_Header::isValid) {
            auto type = typeMap->getType(bim->appliedTo, true);
            auto result = new Util::JsonObject();
            auto l = get(bim->appliedTo);
            if (type->is<IR::Type_HeaderUnion>()) {
                result->emplace("type", "expression");
                auto e = new Util::JsonObject();
                result->emplace("value", e);
                e->emplace("op", "valid_union");
                e->emplace("left", Util::JsonValue::null);
                e->emplace("right", l);
            } else {
                // Treat this as appliedTo.$valid$
                result->emplace("type", "field");
                auto e = mkArrayField(result, "value");
                if (l->is<Util::JsonObject>())
                    e->append(l->to<Util::JsonObject>()->get("value"));
                else
                    e->append(l);
                e->append(V1ModelProperties::validField);
                if (!simpleExpressionsOnly && !leftValue) {
                    // This is set when converting table keys;
                    // for table keys we don't need the casts.
                    auto cast = new Util::JsonObject();
                    auto value = new Util::JsonObject();
                    cast->emplace("type", "expression");
                    cast->emplace("value", value);
                    value->emplace("op", "d2b");  // data to Boolean cast
                    value->emplace("left", Util::JsonValue::null);
                    value->emplace("right", result);
                    result = cast;
                }
            }
            mapExpression(expression, result);
            return;
        }
    }

    BUG("%1%: unhandled case", expression);
}

void ExpressionConverter::postorder(const IR::Cast* expression)  {
    // nothing to do for casts - the ArithmeticFixup pass should have handled them already
    auto j = get(expression->expr);
    mapExpression(expression, j);
}

void ExpressionConverter::postorder(const IR::Constant* expression)  {
    auto result = new Util::JsonObject();
    result->emplace("type", "hexstr");
    auto bitwidth = expression->type->width_bits();
    cstring repr = stringRepr(expression->value, ROUNDUP(bitwidth, 8));
    result->emplace("value", repr);
    if (withConstantWidths) result->emplace("bitwidth", bitwidth);
    mapExpression(expression, result);
}

void ExpressionConverter::postorder(const IR::ArrayIndex* expression)  {
    auto result = new Util::JsonObject();
    result->emplace("type", "header");
    cstring elementAccess;

    // This is can be either a header, which is part of the "headers" parameter
    // or a temporary array.
    if (expression->left->is<IR::Member>()) {
        // This is a header part of the parameters
        auto mem = expression->left->to<IR::Member>();
        auto parentType = typeMap->getType(mem->expr, true);
        BUG_CHECK(parentType->is<IR::Type_StructLike>(), "%1%: expected a struct", parentType);
        auto st = parentType->to<IR::Type_StructLike>();
        auto field = st->getField(mem->member);
        elementAccess = field->controlPlaneName();
    } else if (expression->left->is<IR::PathExpression>()) {
        // This is a temporary variable with type stack.
        auto path = expression->left->to<IR::PathExpression>();
        elementAccess = path->path->name;
    }

    if (!expression->right->is<IR::Constant>()) {
        ::error(ErrorType::ERR_INVALID, "all array indexes must be constant", expression->right);
    } else {
        int index = expression->right->to<IR::Constant>()->asInt();
        elementAccess += "[" + Util::toString(index) + "]";
    }
    result->emplace("value", elementAccess);
    mapExpression(expression, result);
}

/// Non-null if the expression refers to a parameter from the enclosing control
const IR::Parameter*
ExpressionConverter::enclosingParamReference(const IR::Expression* expression) {
    CHECK_NULL(expression);
    if (!expression->is<IR::PathExpression>())
        return nullptr;

    auto pe = expression->to<IR::PathExpression>();
    auto decl = refMap->getDeclaration(pe->path, true);
    auto param = decl->to<IR::Parameter>();
    if (param == nullptr)
        return param;
    if (structure->nonActionParameters.count(param) > 0)
        return param;
    return nullptr;
}

void ExpressionConverter::postorder(const IR::Member* expression)  {
    auto result = new Util::JsonObject();

    auto parentType = typeMap->getType(expression->expr, true);
    cstring fieldName = expression->member.name;
    auto type = typeMap->getType(expression, true);

    if (parentType->is<IR::Type_StructLike>()) {
        auto st = parentType->to<IR::Type_StructLike>();
        auto field = st->getField(expression->member);
        if (field != nullptr)
            // field could be a method call, i.e., isValid.
            fieldName = field->controlPlaneName();
    }

    // handle error
    if (type->is<IR::Type_Error>() && expression->expr->is<IR::TypeNameExpression>()) {
        // this deals with constants that have type 'error'
        result->emplace("type", "hexstr");
        auto decl = type->to<IR::Type_Error>()->getDeclByName(expression->member.name);
        auto errorValue = structure->errorCodesMap.at(decl);
        result->emplace("value", Util::toString(errorValue));
        mapExpression(expression, result);
        return;
    }

    auto param = enclosingParamReference(expression->expr);
    if (param != nullptr) {
        // convert architecture-dependent parameter
        if (auto result = convertParam(param, fieldName)) {
            mapExpression(expression, result);
            return;
        }
        // convert normal parameters
        if (type->is<IR::Type_Stack>()) {
            result->emplace("type", "header_stack");
            result->emplace("value", fieldName);
        } else if (type->is<IR::Type_HeaderUnion>()) {
            result->emplace("type", "header_union");
            result->emplace("value", fieldName);
        } else if (parentType->is<IR::Type_HeaderUnion>()) {
            auto l = get(expression->expr);
            cstring nestedField = fieldName;
            if (l->is<Util::JsonObject>()) {
                auto lv = l->to<Util::JsonObject>()->get("value");
                if (lv->is<Util::JsonValue>()) {
                    // header in union reference ["u", "f"] => "u.f"
                    cstring prefix = lv->to<Util::JsonValue>()->getString();
                    nestedField = prefix + "." + nestedField;
                }
            }
            result->emplace("type", "header");
            result->emplace("value", nestedField);
        } else if (parentType->is<IR::Type_StructLike>() &&
                   (type->is<IR::Type_Bits>() || type->is<IR::Type_Error>() ||
                    type->is<IR::Type_Boolean>())) {
            auto field = parentType->to<IR::Type_StructLike>()->getField(
                expression->member);
            LOG3("looking up field " << field);
            CHECK_NULL(field);
            auto name = ::get(structure->scalarMetadataFields, field);
            BUG_CHECK((name != nullptr), "NULL name: %1%", field->name);
            if (type->is<IR::Type_Bits>() || type->is<IR::Type_Error>() ||
                leftValue || simpleExpressionsOnly) {
                result->emplace("type", "field");
                auto e = mkArrayField(result, "value");
                e->append(scalarsName);
                e->append(name);
            } else if (type->is<IR::Type_Boolean>()) {
                // Boolean variables are stored as ints, so we
                // have to insert a conversion when reading such a
                // variable
                result->emplace("type", "expression");
                auto e = new Util::JsonObject();
                result->emplace("value", e);
                e->emplace("op", "d2b");  // data to Boolean cast
                e->emplace("left", Util::JsonValue::null);
                auto r = new Util::JsonObject();
                e->emplace("right", r);

                r->emplace("type", "field");
                auto a = mkArrayField(r, "value");
                a->append(scalarsName);
                a->append(name);
            }
        } else {
            // This may be wrong, but the caller will handle it properly
            // (e.g., this can be a method, such as packet.lookahead)
            result->emplace("type", "header");
            result->emplace("value", fieldName);
        }
        mapExpression(expression, result);
        return;
    }

    bool done = false;
    if (expression->expr->is<IR::Member>()) {
        auto mem = expression->expr->to<IR::Member>();
        auto memtype = typeMap->getType(mem->expr, true);
        if (memtype->is<IR::Type_Stack>() && mem->member == IR::Type_Stack::next)
            ::error(ErrorType::ERR_UNINITIALIZED, "next field read", mem);
        // array.last.field => type: "stack_field", value: [ array, field ]
        if (memtype->is<IR::Type_Stack>() && mem->member == IR::Type_Stack::last) {
            auto l = get(mem->expr);
            CHECK_NULL(l);
            result->emplace("type", "stack_field");
            auto e = mkArrayField(result, "value");
            if (l->is<Util::JsonObject>())
                e->append(l->to<Util::JsonObject>()->get("value"));
            else
                e->append(l);
            e->append(fieldName);
            done = true;
        }
    }

    if (!done) {
        auto l = get(expression->expr);
        CHECK_NULL(l);
        if (parentType->is<IR::Type_HeaderUnion>()) {
            BUG_CHECK(l->is<Util::JsonObject>(), "Not a JsonObject");
            auto lv = l->to<Util::JsonObject>()->get("value");
            BUG_CHECK(lv->is<Util::JsonValue>(), "Not a JsonValue");
            fieldName = lv->to<Util::JsonValue>()->getString() + "." + fieldName;
            // Each header in a union is allocated a separate header instance.
            // Refer to that instance directly.
            result->emplace("type", "header");
            result->emplace("value", fieldName);
        } else if (parentType->is<IR::Type_Stack>() &&
                   expression->member == IR::Type_Stack::lastIndex) {
            auto l = get(expression->expr);
            CHECK_NULL(l);
            result->emplace("type", "expression");
            auto e = new Util::JsonObject();
            result->emplace("value", e);
            e->emplace("op", "last_stack_index");
            e->emplace("left", Util::JsonValue::null);
            e->emplace("right", l);
        } else {
            const char* fieldRef = parentType->is<IR::Type_Stack>() ? "stack_field" : "field";
            result->emplace("type", fieldRef);
            auto e = mkArrayField(result, "value");
            if (l->is<Util::JsonObject>()) {
                auto lv = l->to<Util::JsonObject>()->get("value");
                if (lv->is<Util::JsonArray>()) {
                    // TODO: is this case still necessary after eliminating nested structs?
                    // nested struct reference [ ["m", "f"], "x" ] => [ "m", "f.x" ]
                    auto array = lv->to<Util::JsonArray>();
                    BUG_CHECK(array->size() == 2, "expected 2 elements");
                    auto first = array->at(0);
                    auto second = array->at(1);
                    BUG_CHECK(second->is<Util::JsonValue>(), "expected a value");
                    e->append(first);
                    cstring nestedField = second->to<Util::JsonValue>()->getString();
                    nestedField += "." + fieldName;
                    e->append(nestedField);
                } else if (lv->is<Util::JsonValue>()) {
                    e->append(lv);
                    e->append(fieldName);
                } else {
                    BUG("%1%: Unexpected json", lv);
                }
            } else {
                e->append(l);
                e->append(fieldName);
            }
            if (!simpleExpressionsOnly && !leftValue && type->is<IR::Type_Boolean>()) {
                auto cast = new Util::JsonObject();
                auto value = new Util::JsonObject();
                cast->emplace("type", "expression");
                cast->emplace("value", value);
                value->emplace("op", "d2b");  // data to Boolean cast
                value->emplace("left", Util::JsonValue::null);
                value->emplace("right", result);
                result = cast;
            }
        }
    }
    mapExpression(expression, result);
}

Util::IJson* ExpressionConverter::fixLocal(Util::IJson* json) {
    if (json->is<Util::JsonObject>()) {
        auto jo = json->to<Util::JsonObject>();
        auto to = jo->get("type");
        if (to != nullptr && to->to<Util::JsonValue>() != nullptr &&
            (*to->to<Util::JsonValue>()) == "runtime_data") {
            auto result = new Util::JsonObject();
            result->emplace("type", "local");
            result->emplace("value", jo->get("value"));
            return result;
        }
    }
    return json;
}

void ExpressionConverter::postorder(const IR::Mux* expression)  {
    auto result = new Util::JsonObject();
    mapExpression(expression, result);
    if (simpleExpressionsOnly) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "expression too complex", expression);
        return;
    }

    result->emplace("type", "expression");
    auto e = new Util::JsonObject();
    result->emplace("value", e);
    e->emplace("op", "?");
    auto l = get(expression->e1);
    e->emplace("left", fixLocal(l));
    auto r = get(expression->e2);
    e->emplace("right", fixLocal(r));
    auto c = get(expression->e0);
    e->emplace("cond", fixLocal(c));
}

void ExpressionConverter::postorder(const IR::IntMod* expression)  {
    auto result = new Util::JsonObject();
    mapExpression(expression, result);
    result->emplace("type", "expression");
    auto e = new Util::JsonObject();
    result->emplace("value", e);
    e->emplace("op", "two_comp_mod");
    auto l = get(expression->expr);
    e->emplace("left", fixLocal(l));
    auto r = new Util::JsonObject();
    r->emplace("type", "hexstr");
    cstring repr = stringRepr(expression->width);
    r->emplace("value", repr);
    e->emplace("right", r);
}

void ExpressionConverter::postorder(const IR::Operation_Binary* expression)  {
    binary(expression);
}

void ExpressionConverter::binary(const IR::Operation_Binary* expression) {
    auto result = new Util::JsonObject();
    mapExpression(expression, result);
    if (simpleExpressionsOnly) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "expression too complex", expression);
        return;
    }

    result->emplace("type", "expression");
    auto e = new Util::JsonObject();
    result->emplace("value", e);
    cstring op = expression->getStringOp();
    if (op == "&&")
        op = "and";
    else if (op == "||")
        op = "or";
    e->emplace("op", op);
    auto l = get(expression->left);
    e->emplace("left", fixLocal(l));
    auto r = get(expression->right);
    e->emplace("right", fixLocal(r));
}

void ExpressionConverter::saturated_binary(const IR::Operation_Binary* expression) {
    // This should never happen if we correctly typecheck the program
    BUG_CHECK(expression->type->is<IR::Type_Bits>(), "saturated arithmetic requires bit types");

    auto result = new Util::JsonObject();
    mapExpression(expression, result);
    result->emplace("type", "expression");
    auto e = new Util::JsonObject();
    result->emplace("value", e);
    auto eType = expression->type->to<IR::Type_Bits>();
    CHECK_NULL(eType);
    auto opType = eType->isSigned ? "sat_cast" : "usat_cast";
    e->emplace("op", opType);

    // the left operand is the binary expression, but as a simple add/sub
    auto eLeft = new Util::JsonObject();
    eLeft->emplace("type", "expression");
    auto e1 = new Util::JsonObject();
    e1->emplace("op", expression->getStringOp() == "|+|" ? "+" : "-");
    e1->emplace("left", fixLocal(get(expression->left)));
    e1->emplace("right", fixLocal(get(expression->right)));
    eLeft->emplace("value", e1);
    e->emplace("left", eLeft);

    // the right operand is the width of the type
    auto r = new Util::JsonObject();
    r->emplace("type", "hexstr");
    cstring repr = stringRepr(eType->width_bits());
    r->emplace("value", repr);
    e->emplace("right", r);
}

void ExpressionConverter::postorder(const IR::ListExpression* expression)  {
    auto result = new Util::JsonArray();
    mapExpression(expression, result);
    if (simpleExpressionsOnly) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "expression too complex", expression);
        return;
    }

    for (auto e : expression->components) {
        auto t = get(e);
        result->append(t);
    }
}

void ExpressionConverter::postorder(const IR::StructInitializerExpression* expression)  {
    // Handle like a ListExpression
    auto result = new Util::JsonArray();
    mapExpression(expression, result);
    if (simpleExpressionsOnly) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "expression too complex", expression);
        return;
    }

    for (auto e : expression->components) {
        auto t = get(e->expression);
        result->append(t);
    }
}

void ExpressionConverter::postorder(const IR::Operation_Unary* expression)  {
    auto result = new Util::JsonObject();
    mapExpression(expression, result);
    if (simpleExpressionsOnly) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "expression too complex", expression);
        return;
    }

    result->emplace("type", "expression");
    auto e = new Util::JsonObject();
    result->emplace("value", e);
    cstring op = expression->getStringOp();
    if (op == "!")
        op = "not";
    e->emplace("op", op);
    e->emplace("left", Util::JsonValue::null);
    auto r = get(expression->expr);
    e->emplace("right", fixLocal(r));
}

void ExpressionConverter::postorder(const IR::PathExpression* expression)  {
    // This is useful for action bodies mostly
    auto decl = refMap->getDeclaration(expression->path, true);
    if (auto param = decl->to<IR::Parameter>()) {
        if (structure->nonActionParameters.find(param) !=
            structure->nonActionParameters.end()) {
            auto type = typeMap->getType(param, true);
            if (type->is<IR::Type_StructLike>()) {
                auto result = new Util::JsonObject();
                result->emplace("type", "header");
                result->emplace("value", param->name.name);
                mapExpression(expression, result);
            } else {
                mapExpression(expression, new Util::JsonValue(param->name.name));
            }
            return;
        }
        auto result = new Util::JsonObject();
        result->emplace("type", "runtime_data");
        unsigned paramIndex = ::get(&structure->index, param);
        result->emplace("value", paramIndex);
        mapExpression(expression, result);
    } else if (auto var = decl->to<IR::Declaration_Variable>()) {
        LOG3("Variable to json " << var);
        auto result = new Util::JsonObject();
        auto type = typeMap->getType(var, true);
        if (type->is<IR::Type_StructLike>()) {
            result->emplace("type", "header");
            result->emplace("value", var->name);
        } else if (type->is<IR::Type_Bits>() ||
                   (type->is<IR::Type_Boolean>() && (leftValue || simpleExpressionsOnly))) {
            // no conversion d2b when writing (leftValue is true) to a boolean
            result->emplace("type", "field");
            auto e = mkArrayField(result, "value");
            e->append(scalarsName);
            e->append(var->name);
        } else if (type->is<IR::Type_Varbits>()) {
            // varbits are synthesized in separate metadata instances
            // with a single field each, where the field is named
            // "field".
            result->emplace("type", "field");
            auto e = mkArrayField(result, "value");
            e->append(var->name);
            e->append("field");
        } else if (type->is<IR::Type_Boolean>()) {
            // Boolean variables are stored as ints, so we have to insert a conversion when
            // reading such a variable
            result->emplace("type", "expression");
            auto e = new Util::JsonObject();
            result->emplace("value", e);
            e->emplace("op", "d2b");  // data to Boolean cast
            e->emplace("left", Util::JsonValue::null);
            auto r = new Util::JsonObject();
            e->emplace("right", r);
            r->emplace("type", "field");
            auto f = mkArrayField(r, "value");
            f->append(scalarsName);
            f->append(var->name);
        } else if (type->is<IR::Type_Stack>()) {
            result->emplace("type", "header_stack");
            result->emplace("value", var->name);
        } else if (type->is<IR::Type_Error>()) {
            result->emplace("type", "field");
            auto f = mkArrayField(result, "value");
            f->append(scalarsName);
            f->append(var->name);
        } else {
            BUG("%1%: type not yet handled", type);
        }
        mapExpression(expression, result);
    }
}

void ExpressionConverter::postorder(const IR::TypeNameExpression* expression)  {
    (void)expression;
}

void ExpressionConverter::postorder(const IR::Expression* expression)  {
    BUG("%1%: Unhandled case", expression);
}

// doFixup = true -> insert masking operations for proper arithmetic implementation
// see below for wrap
Util::IJson*
ExpressionConverter::convert(const IR::Expression* e, bool doFixup, bool wrap, bool convertBool) {
    const IR::Expression *expr = e;
    if (doFixup) {
        ArithmeticFixup af(typeMap);
        auto r = e->apply(af);
        CHECK_NULL(r);
        expr = r->to<IR::Expression>();
        CHECK_NULL(expr);
    }
    expr->apply(*this);
    auto result = ::get(map, expr->to<IR::Expression>());
    if (result == nullptr)
        BUG("%1%: Could not convert expression", e);

    if (convertBool) {
        auto obj = new Util::JsonObject();
        obj->emplace("type", "expression");
        auto conv = new Util::JsonObject();
        obj->emplace("value", conv);
        conv->emplace("op", "b2d");  // boolean to data cast
        conv->emplace("left", Util::JsonValue::null);
        conv->emplace("right", result);
        result = obj;
    }

    std::set<cstring> to_wrap({"expression", "stack_field"});

    // This is weird, but that's how it is: expression and stack_field must be wrapped in
    // another outer object. In a future version of the bmv2 JSON, this will not be needed
    // anymore as expressions will be treated in a more uniform way.
    if (wrap && result->is<Util::JsonObject>()) {
        auto to = result->to<Util::JsonObject>()->get("type");
        if (to != nullptr && to->to<Util::JsonValue>() != nullptr) {
            auto jv = *to->to<Util::JsonValue>();
            if (jv.isString() && to_wrap.find(jv.getString()) != to_wrap.end()) {
                auto rwrap = new Util::JsonObject();
                rwrap->emplace("type", "expression");
                rwrap->emplace("value", result);
                result = rwrap;
            }
        }
    }
    return result;
}

Util::IJson* ExpressionConverter::convertLeftValue(const IR::Expression* e) {
    leftValue = true;
    const IR::Expression *expr = e;
    ArithmeticFixup af(typeMap);
    auto r = e->apply(af);
    CHECK_NULL(r);
    expr = r->to<IR::Expression>();
    CHECK_NULL(expr);
    expr->apply(*this);
    auto result = ::get(map, expr->to<IR::Expression>());
    if (result == nullptr)
        BUG("%1%: Could not convert expression", e);
    leftValue = false;
    return result;
}

Util::IJson* ExpressionConverter::convertWithConstantWidths(const IR::Expression* e) {
    withConstantWidths = true;
    auto result = convert(e);
    withConstantWidths = false;
    return result;
}

}  // namespace BMV2
