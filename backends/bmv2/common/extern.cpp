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

#include "extern.h"
#include "frontends/p4/fromv1.0/v1model.h"

namespace BMV2 {

std::map<cstring, ExternConverter *> *ExternConverter::cvtForType = nullptr;

void ExternConverter::registerExternConverter(cstring name, ExternConverter* cvt) {
    static std::map<cstring, ExternConverter *> map;
    cvtForType = &map;
    LOG3("registered extern " << name);
    map[name] = cvt;
}

ExternConverter* ExternConverter::get(cstring type) {
    static ExternConverter defaultCvt;
    if (cvtForType && cvtForType->count(type))
        return cvtForType->at(type);
    return &defaultCvt;
}

Util::IJson*
ExternConverter::cvtExternObject(ConversionContext* ctxt,
                                 const P4::ExternMethod* em,
                                 const IR::MethodCallExpression* mc,
                                 const IR::StatOrDecl* s,
                                 const bool& emitExterns) {
    return get(em)->convertExternObject(ctxt, em, mc, s, emitExterns);
}

void
ExternConverter::cvtExternInstance(ConversionContext* ctxt,
                                   const IR::Declaration* c,
                                   const IR::ExternBlock* eb,
                                   const bool& emitExterns) {
    get(eb)->convertExternInstance(ctxt, c, eb, emitExterns);
}

Util::IJson*
ExternConverter::cvtExternFunction(ConversionContext* ctxt,
                                   const P4::ExternFunction* ef,
                                   const IR::MethodCallExpression* mc,
                                   const IR::StatOrDecl* s,
                                   const bool emitExterns) {
    return get(ef)->convertExternFunction(ctxt, ef, mc, s, emitExterns);
}

Util::IJson*
ExternConverter::convertExternObject(ConversionContext* ctxt,
                                     const P4::ExternMethod* em,
                                     const IR::MethodCallExpression* mc,
                                     const IR::StatOrDecl*,
                                     const bool& emitExterns) {
    if (emitExterns) {
        auto primitive = mkPrimitive("_" + em->originalExternType->name +
                                         "_" + em->method->name);
        auto parameters = mkParameters(primitive);
        primitive->emplace_non_null("source_info", mc->sourceInfoJsonObj());
        auto etr = new Util::JsonObject();
        etr->emplace("type", "extern");
        etr->emplace("value", em->object->controlPlaneName());
        parameters->append(etr);
        for (auto arg : *mc->arguments) {
            auto args = ctxt->conv->convert(arg->expression);
            parameters->append(args);
        }
        return primitive;
    } else {
        ::error(ErrorType::ERR_UNKNOWN, "Unknown extern method %1% from type %2%",
                em->method->name, em->originalExternType->name);
        return nullptr;
    }
}

/// This method is invoked for all externs that do not have a registered
/// conversion, i.e., unknown by the architecture.
void
ExternConverter::convertExternInstance(ConversionContext* ctxt,
                                       const IR::Declaration* decl,
                                       const IR::ExternBlock* eb,
                                       const bool& emitExterns) {
    if (!emitExterns) {
        ::error(ErrorType::ERR_UNKNOWN, "extern instance", eb->type->name);
        return;
    }
    auto attrs = new Util::JsonArray();
    auto params = eb->getConstructorParameters();
    for (auto param : params->parameters) {
        auto val = eb->getParameterValue(param->name);
        cstring type;
        cstring value;
        if (auto cst = val->to<IR::Constant>()) {
            type = "hexstr";
            value = Util::toString(&cst->value, 16);
        } else if (auto str = val->to<IR::StringLiteral>()) {
            type = "string";
            value = str->value;
        } else {
            modelError("%1%: parameter type not unsupported", param->type);
            continue;
        }
        ctxt->json->add_extern_attribute(param->name, type, value, attrs);
    }
    ctxt->json->add_extern(decl->controlPlaneName(), eb->type->getName(), attrs);
}

Util::IJson*
ExternConverter::convertExternFunction(ConversionContext* ctxt,
                                       const P4::ExternFunction* ef,
                                       const IR::MethodCallExpression* mc,
                                       const IR::StatOrDecl* s,
                                       const bool emitExterns) {
    if (!emitExterns) {
        ::error(ErrorType::ERR_UNKNOWN, "extern function", ef->method->name);
        return nullptr;
    }
    auto primitive = mkPrimitive(ef->method->name);
    primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
    auto parameters = mkParameters(primitive);
    for (auto arg : *mc->arguments) {
        auto args = ctxt->conv->convert(arg->expression);
        parameters->append(args);
    }
    return primitive;
}

void
ExternConverter::modelError(const char* format, const IR::Node* node) const {
    cstring errMsg = cstring(format) +
                     ". Are you using an up-to-date v1model.p4?";
    ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, errMsg.c_str(), node);
}

void
ExternConverter::addToFieldList(ConversionContext* ctxt,
                                const IR::Expression* expr, Util::JsonArray* fl) {
    if (auto le = expr->to<IR::ListExpression>()) {
        for (auto e : le->components) {
            addToFieldList(ctxt, e, fl);
        }
        return;
    } else if (auto si = expr->to<IR::StructInitializerExpression>()) {
        for (auto e : si->components) {
            addToFieldList(ctxt, e->expression, fl);
        }
        return;
    }

    auto type = ctxt->typeMap->getType(expr, true);
    if (type->is<IR::Type_StructLike>()) {
        // recursively add all fields
        auto st = type->to<IR::Type_StructLike>();
        for (auto f : st->fields) {
            auto member = new IR::Member(expr, f->name);
            ctxt->typeMap->setType(member, ctxt->typeMap->getType(f, true));
            addToFieldList(ctxt, member, fl);
        }
        return;
    }

    bool simple = ctxt->conv->simpleExpressionsOnly;
    ctxt->conv->simpleExpressionsOnly = true;  // we do not want casts d2b in field_lists
    auto j = ctxt->conv->convert(expr);
    ctxt->conv->simpleExpressionsOnly = simple;  // restore state
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

int
ExternConverter::createFieldList(ConversionContext* ctxt,
                                 const IR::Expression* expr, cstring group,
                                 cstring listName, Util::JsonArray* field_lists) {
    auto fl = new Util::JsonObject();
    field_lists->append(fl);
    int id = nextId(group);
    fl->emplace("id", id);
    fl->emplace("name", listName);
    fl->emplace_non_null("source_info", expr->sourceInfoJsonObj());
    auto elements = mkArrayField(fl, "elements");
    addToFieldList(ctxt, expr, elements);
    return id;
}

cstring
ExternConverter::createCalculation(ConversionContext* ctxt,
                                   cstring algo, const IR::Expression* fields,
                                   Util::JsonArray* calculations, bool withPayload,
                                   const IR::Node* sourcePositionNode = nullptr) {
    cstring calcName = ctxt->refMap->newName("calc_");
    auto calc = new Util::JsonObject();
    calc->emplace("name", calcName);
    calc->emplace("id", nextId("calculations"));
    if (sourcePositionNode != nullptr)
        calc->emplace_non_null("source_info", sourcePositionNode->sourceInfoJsonObj());
    calc->emplace("algo", algo);
    if (!fields->is<IR::ListExpression>()) {
        // expand it into a list
        auto list = new IR::ListExpression({});
        auto type = ctxt->typeMap->getType(fields, true);
        if (!type->is<IR::Type_StructLike>()) {
            modelError("%1%: expected a struct", fields);
            return calcName;
        }
        for (auto f : type->to<IR::Type_StructLike>()->fields) {
            auto e = new IR::Member(fields, f->name);
            auto ftype = ctxt->typeMap->getType(f);
            ctxt->typeMap->setType(e, ftype);
            list->push_back(e);
        }
        fields = list;
        ctxt->typeMap->setType(fields, type);
    }
    auto jright = ctxt->conv->convertWithConstantWidths(fields);
    if (withPayload) {
        auto array = jright->to<Util::JsonArray>();
        BUG_CHECK(array, "expected a JSON array");
        auto payload = new Util::JsonObject();
        payload->emplace("type", "payload");
        payload->emplace("value", (Util::IJson*)nullptr);
        array->append(payload);
    }
    calc->emplace("input", jright);
    calculations->append(calc);
    return calcName;
}

cstring
ExternConverter::convertHashAlgorithm(cstring algorithm) {
    cstring result;
    if (algorithm == P4V1::V1Model::instance.algorithm.crc32.name)
        result = "crc32";
    else if (algorithm == P4V1::V1Model::instance.algorithm.crc32_custom.name)
        result = "crc32_custom";
    else if (algorithm == P4V1::V1Model::instance.algorithm.crc16.name)
        result = "crc16";
    else if (algorithm == P4V1::V1Model::instance.algorithm.crc16_custom.name)
        result = "crc16_custom";
    else if (algorithm == P4V1::V1Model::instance.algorithm.random.name)
        result = "random";
    else if (algorithm == P4V1::V1Model::instance.algorithm.identity.name)
        result = "identity";
    else if (algorithm == P4V1::V1Model::instance.algorithm.csum16.name)
        result = "csum16";
    else if (algorithm == P4V1::V1Model::instance.algorithm.xor16.name)
        result = "xor16";
    else
        ::error("Unsupported algorithm %1%", algorithm);
    return result;
}

ExternConverter_assert ExternConverter_assert::singleton;
ExternConverter_assume ExternConverter_assume::singleton;

Util::IJson*
ExternConverter::convertAssertAssume(ConversionContext* ctxt,
    const IR::MethodCallExpression* methodCall, const P4::ExternFunction* ef) {
     if (methodCall->arguments->size() != 1) {
        ::error("Expected 1 arguments for %1%", methodCall);
        return nullptr;
    }
    auto primitive = mkPrimitive(ef->method->name.name);
    auto parameters = mkParameters(primitive);
    auto cond = methodCall->arguments->at(0);
    // wrap expression in an additional JSON expression block
    // cast the result of expression to b2d
    auto jsonExpr = ctxt->conv->convert(cond->expression, true, true, true);
    parameters->append(jsonExpr);
    primitive->emplace_non_null("source_info", methodCall->sourceInfoJsonObj());
    return primitive;
}

Util::IJson* ExternConverter_assert::convertExternFunction(
    UNUSED ConversionContext* ctxt, UNUSED const P4::ExternFunction* ef,
    UNUSED const IR::MethodCallExpression* mc, UNUSED const IR::StatOrDecl* s,
    UNUSED const bool emitExterns) {
    return convertAssertAssume(ctxt, mc, ef);
}

Util::IJson* ExternConverter_assume::convertExternFunction(
    UNUSED ConversionContext* ctxt, UNUSED const P4::ExternFunction* ef,
    UNUSED const IR::MethodCallExpression* mc, UNUSED const IR::StatOrDecl* s,
    UNUSED const bool emitExterns) {
    return ExternConverter::convertAssertAssume(ctxt, mc, ef);
}

}  // namespace BMV2
