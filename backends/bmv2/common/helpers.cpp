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

#include "helpers.h"

#include <ostream>
#include <string>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/traits/explicit_conversion.hpp>

#include "backends/bmv2/common/JsonObjects.h"
#include "backends/bmv2/common/controlFlowGraph.h"
#include "backends/bmv2/common/expression.h"
#include "ir/indexed_vector.h"
#include "ir/vector.h"
#include "lib/error.h"
#include "lib/exceptions.h"

namespace BMV2 {

/// constant definition for bmv2
const cstring MatchImplementation::selectorMatchTypeName = "selector";
const cstring MatchImplementation::rangeMatchTypeName = "range";
const cstring MatchImplementation::optionalMatchTypeName = "optional";
const unsigned TableAttributes::defaultTableSize = 1024;
const cstring V1ModelProperties::jsonMetadataParameterName = "standard_metadata";
const cstring V1ModelProperties::validField = "$valid$";

Util::IJson *nodeName(const CFG::Node *node) {
    if (node->name.isNullOrEmpty())
        return Util::JsonValue::null;
    else
        return new Util::JsonValue(node->name);
}

Util::JsonArray *mkArrayField(Util::JsonObject *parent, cstring name) {
    auto result = new Util::JsonArray();
    parent->emplace(name, result);
    return result;
}

Util::JsonArray *mkParameters(Util::JsonObject *object) {
    return mkArrayField(object, "parameters");
}

Util::JsonObject *mkPrimitive(cstring name, Util::JsonArray *appendTo) {
    auto result = new Util::JsonObject();
    result->emplace("op", name);
    appendTo->append(result);
    return result;
}

Util::JsonObject *mkPrimitive(cstring name) {
    auto result = new Util::JsonObject();
    result->emplace("op", name);
    return result;
}

cstring stringRepr(big_int value, unsigned bytes) {
    cstring sign = "";
    std::stringstream r;
    cstring filler = "";
    if (value < 0) {
        value = -value;
        sign = "-";
    }
    r << std::hex << value;

    if (bytes > 0) {
        int digits = bytes * 2 - r.str().size();
        BUG_CHECK(digits >= 0, "Cannot represent %1% on %2% bytes", value, bytes);
        filler = std::string(digits, '0');
    }
    return sign + "0x" + filler + r.str();
}

unsigned nextId(cstring group) {
    static std::map<cstring, unsigned> counters;
    return counters[group]++;
}

void ConversionContext::addToFieldList(const IR::Expression *expr, Util::JsonArray *fl) {
    if (auto le = expr->to<IR::ListExpression>()) {
        for (auto e : le->components) {
            addToFieldList(e, fl);
        }
        return;
    } else if (auto si = expr->to<IR::StructExpression>()) {
        for (auto e : si->components) {
            addToFieldList(e->expression, fl);
        }
        return;
    }

    auto type = typeMap->getType(expr, true);
    if (type->is<IR::Type_StructLike>()) {
        // recursively add all fields
        auto st = type->to<IR::Type_StructLike>();
        for (auto f : st->fields) {
            auto member = new IR::Member(expr, f->name);
            typeMap->setType(member, typeMap->getType(f, true));
            addToFieldList(member, fl);
        }
        return;
    }

    bool simple = conv->simpleExpressionsOnly;
    conv->simpleExpressionsOnly = true;  // we do not want casts d2b in field_lists
    auto j = conv->convert(expr);
    conv->simpleExpressionsOnly = simple;  // restore state
    if (auto jo = j->to<Util::JsonObject>()) {
        if (auto t = jo->get("type")) {
            if (auto type = t->to<Util::JsonValue>()) {
                if (*type == "runtime_data") {
                    // Can't have runtime_data in field lists -- need hexstr instead
                    auto val = jo->get("value")->to<Util::JsonValue>();
                    j = jo = new Util::JsonObject();
                    jo->emplace("type", "hexstr");
                    jo->emplace("value", stringRepr(val->getValue()));
                }
            }
        }
    }
    fl->append(j);
}

int ConversionContext::createFieldList(const IR::Expression *expr, cstring listName, bool learn) {
    cstring group;
    auto fl = new Util::JsonObject();
    if (learn) {
        group = "learn_lists";
        json->learn_lists->append(fl);
    } else {
        group = "field_lists";
        json->field_lists->append(fl);
    }
    int id = nextId(group);
    fl->emplace("id", id);
    fl->emplace("name", listName);
    fl->emplace_non_null("source_info", expr->sourceInfoJsonObj());
    auto elements = mkArrayField(fl, "elements");
    addToFieldList(expr, elements);
    return id;
}

void ConversionContext::modelError(const char *format, const IR::Node *node) {
    ::error(format, node);
    ::error("Are you using an up-to-date v1model.p4?");
}

cstring ConversionContext::createCalculation(cstring algo, const IR::Expression *fields,
                                             Util::JsonArray *calculations, bool withPayload,
                                             const IR::Node *sourcePositionNode = nullptr) {
    cstring calcName = refMap->newName("calc_");
    auto calc = new Util::JsonObject();
    calc->emplace("name", calcName);
    calc->emplace("id", nextId("calculations"));
    if (sourcePositionNode != nullptr)
        calc->emplace_non_null("source_info", sourcePositionNode->sourceInfoJsonObj());
    calc->emplace("algo", algo);
    auto listFields = convertToList(fields, typeMap);
    if (!listFields) {
        modelError("%1%: expected a struct", fields);
        return calcName;
    }
    auto jright = conv->convertWithConstantWidths(listFields);
    if (withPayload) {
        auto array = jright->to<Util::JsonArray>();
        BUG_CHECK(array, "expected a JSON array");
        auto payload = new Util::JsonObject();
        payload->emplace("type", "payload");
        payload->emplace("value", (Util::IJson *)nullptr);
        array->append(payload);
    }
    calc->emplace("input", jright);
    calculations->append(calc);
    return calcName;
}

/// Converts expr into a ListExpression or returns nullptr if not
/// possible
const IR::ListExpression *convertToList(const IR::Expression *expr, P4::TypeMap *typeMap) {
    if (auto l = expr->to<IR::ListExpression>()) return l;

    // expand it into a list
    auto list = new IR::ListExpression({});
    auto type = typeMap->getType(expr, true);
    auto st = type->to<IR::Type_StructLike>();
    if (!st) {
        return nullptr;
    }
    if (auto se = expr->to<IR::StructExpression>()) {
        for (auto f : se->components) list->push_back(f->expression);
    } else {
        for (auto f : st->fields) {
            auto e = new IR::Member(expr, f->name);
            auto ftype = typeMap->getType(f);
            typeMap->setType(e, ftype);
            list->push_back(e);
        }
    }
    typeMap->setType(list, type);
    return list;
}

}  // namespace BMV2
