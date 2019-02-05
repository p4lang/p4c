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

/**
 * This file implements the simple switch model
 */

#include <algorithm>
#include <cstring>
#include <set>
#include "backends/bmv2/common/annotations.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "simpleSwitch.h"

using BMV2::mkArrayField;
using BMV2::mkParameters;
using BMV2::mkPrimitive;
using BMV2::nextId;
using BMV2::stringRepr;

namespace BMV2 {

void ParseV1Architecture::modelError(const char* format, const IR::Node* node) {
    ::error(format, node);
    ::error("Are you using an up-to-date v1model.p4?");
}

bool ParseV1Architecture::preorder(const IR::PackageBlock* main) {
    auto prsr = main->findParameterValue(v1model.sw.parser.name);
    if (prsr == nullptr || !prsr->is<IR::ParserBlock>()) {
        modelError("%1%: main package  match the expected model", main);
        return false;
    }
    structure->block_type.emplace(prsr->to<IR::ParserBlock>()->container, V1_PARSER);

    auto ingress = main->findParameterValue(v1model.sw.ingress.name);
    if (ingress == nullptr || !ingress->is<IR::ControlBlock>()) {
        modelError("%1%: main package does not match the expected model", main);
        return false;
    }
    auto ingress_name = ingress->to<IR::ControlBlock>()->container->name;
    structure->block_type.emplace(ingress->to<IR::ControlBlock>()->container, V1_INGRESS);
    structure->pipeline_controls.emplace(ingress_name);

    auto verify = main->findParameterValue(v1model.sw.verify.name);
    if (verify == nullptr || !verify->is<IR::ControlBlock>()) {
        modelError("%1%: main package does not match the expected model", main);
        return false;
    }
    structure->block_type.emplace(verify->to<IR::ControlBlock>()->container, V1_VERIFY);
    structure->non_pipeline_controls.emplace(verify->to<IR::ControlBlock>()->container->name);

    auto egress = main->findParameterValue(v1model.sw.egress.name);
    if (egress == nullptr || !egress->is<IR::ControlBlock>()) {
        modelError("%1%: main package does not match the expected model", main);
        return false;
    }
    auto egress_name = egress->to<IR::ControlBlock>()->container->name;
    structure->block_type.emplace(egress->to<IR::ControlBlock>()->container, V1_EGRESS);
    structure->pipeline_controls.emplace(egress_name);

    auto compute = main->findParameterValue(v1model.sw.compute.name);
    if (compute == nullptr || !compute->is<IR::ControlBlock>()) {
        modelError("%1%: main package does not match the expected model", main);
        return false;
    }
    structure->block_type.emplace(compute->to<IR::ControlBlock>()->container, V1_COMPUTE);
    structure->non_pipeline_controls.emplace(compute->to<IR::ControlBlock>()->container->name);

    auto deparser = main->findParameterValue(v1model.sw.deparser.name);
    if (deparser == nullptr || !deparser->is<IR::ControlBlock>()) {
        modelError("%1%: main package  match the expected model", main);
        return false;
    }
    structure->block_type.emplace(deparser->to<IR::ControlBlock>()->container, V1_DEPARSER);
    structure->non_pipeline_controls.emplace(deparser->to<IR::ControlBlock>()->container->name);

    return false;
}

ExternConverter_clone ExternConverter_clone::singleton;
ExternConverter_clone3 ExternConverter_clone3::singleton;
ExternConverter_hash ExternConverter_hash::singleton;
ExternConverter_digest ExternConverter_digest::singleton;
ExternConverter_resubmit ExternConverter_resubmit::singleton;
ExternConverter_recirculate ExternConverter_recirculate::singleton;
ExternConverter_mark_to_drop ExternConverter_mark_to_drop::singleton;
ExternConverter_random ExternConverter_random::singleton;
ExternConverter_truncate ExternConverter_truncate::singleton;
ExternConverter_register ExternConverter_register::singleton;
ExternConverter_counter ExternConverter_counter::singleton;
ExternConverter_meter ExternConverter_meter::singleton;
ExternConverter_direct_counter ExternConverter_direct_counter::singleton;
ExternConverter_direct_meter ExternConverter_direct_meter::singleton;
ExternConverter_action_profile ExternConverter_action_profile::singleton;
ExternConverter_action_selector ExternConverter_action_selector::singleton;

Util::IJson* ExternConverter_clone::convertExternFunction(
    UNUSED ConversionContext* ctxt, UNUSED const P4::ExternFunction* ef,
    UNUSED const IR::MethodCallExpression* mc, UNUSED const IR::StatOrDecl* s,
    UNUSED const bool emitExterns) {
    int id = -1;
    if (mc->arguments->size() != 2) {
        modelError("Expected 2 arguments for %1%", mc);
        return nullptr;
    }
    cstring name = ctxt->refMap->newName("fl");
    auto emptylist = new IR::ListExpression({});
    id = createFieldList(ctxt, emptylist, "field_lists", name, ctxt->json->field_lists);

    auto cloneType = mc->arguments->at(0);
    auto ei = P4::EnumInstance::resolve(cloneType->expression, ctxt->typeMap);
    if (ei == nullptr) {
        modelError("%1%: must be a constant on this target", cloneType);
        return nullptr;
    }
    cstring prim = ei->name == "I2E" ? "clone_ingress_pkt_to_egress" :
                   "clone_egress_pkt_to_egress";
    auto session = ctxt->conv->convert(mc->arguments->at(1)->expression);
    auto primitive = mkPrimitive(prim);
    auto parameters = mkParameters(primitive);
    primitive->emplace_non_null("source_info", mc->sourceInfoJsonObj());
    parameters->append(session);

    if (id >= 0) {
        auto cst = new IR::Constant(id);
        ctxt->typeMap->setType(cst, IR::Type_Bits::get(32));
        auto jcst = ctxt->conv->convert(cst);
        parameters->append(jcst);
    }
    return primitive;
}

Util::IJson* ExternConverter_clone3::convertExternFunction(
    UNUSED ConversionContext* ctxt, UNUSED const P4::ExternFunction* ef,
    UNUSED const IR::MethodCallExpression* mc, UNUSED const IR::StatOrDecl* s,
    UNUSED const bool emitExterns) {
    (void) v1model.clone.clone3.name;
    int id = -1;
    if (mc->arguments->size() != 3) {
        modelError("Expected 3 arguments for %1%", mc);
        return nullptr;
    }
    cstring name = ctxt->refMap->newName("fl");
    id = createFieldList(ctxt, mc->arguments->at(2)->expression, "field_lists", name,
                         ctxt->json->field_lists);
    auto cloneType = mc->arguments->at(0);
    auto ei = P4::EnumInstance::resolve(cloneType->expression, ctxt->typeMap);
    if (ei == nullptr) {
        modelError("%1%: must be a constant on this target", cloneType);
        return nullptr;
    }
    cstring prim = ei->name == "I2E" ? "clone_ingress_pkt_to_egress" :
                   "clone_egress_pkt_to_egress";
    auto session = ctxt->conv->convert(mc->arguments->at(1)->expression);
    auto primitive = mkPrimitive(prim);
    auto parameters = mkParameters(primitive);
    primitive->emplace_non_null("source_info", mc->sourceInfoJsonObj());
    parameters->append(session);

    if (id >= 0) {
        auto cst = new IR::Constant(id);
        ctxt->typeMap->setType(cst, IR::Type_Bits::get(32));
        auto jcst = ctxt->conv->convert(cst);
        parameters->append(jcst);
    }
    return primitive;
}

Util::IJson* ExternConverter_hash::convertExternFunction(
    UNUSED ConversionContext* ctxt, UNUSED const P4::ExternFunction* ef,
    UNUSED const IR::MethodCallExpression* mc, UNUSED const IR::StatOrDecl* s,
    UNUSED const bool emitExterns) {
    static std::set<cstring> supportedHashAlgorithms = {
        v1model.algorithm.crc32.name, v1model.algorithm.crc32_custom.name,
        v1model.algorithm.crc16.name, v1model.algorithm.crc16_custom.name,
        v1model.algorithm.random.name, v1model.algorithm.identity.name,
        v1model.algorithm.csum16.name, v1model.algorithm.xor16.name
    };

    if (mc->arguments->size() != 5) {
        modelError("Expected 5 arguments for %1%", mc);
        return nullptr;
    }
    auto primitive = mkPrimitive("modify_field_with_hash_based_offset");
    auto parameters = mkParameters(primitive);
    primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
    auto dest = ctxt->conv->convert(mc->arguments->at(0)->expression);
    parameters->append(dest);
    auto base = ctxt->conv->convert(mc->arguments->at(2)->expression);
    parameters->append(base);
    auto calculation = new Util::JsonObject();
    auto ei = P4::EnumInstance::resolve(mc->arguments->at(1)->expression, ctxt->typeMap);
    CHECK_NULL(ei);
    if (supportedHashAlgorithms.find(ei->name) == supportedHashAlgorithms.end()) {
        ::error("%1%: unexpected algorithm", ei->name);
        return nullptr;
    }
    auto fields = mc->arguments->at(3);
    auto calcName = createCalculation(ctxt, ei->name, fields->expression, ctxt->json->calculations,
                                      false, nullptr);
    calculation->emplace("type", "calculation");
    calculation->emplace("value", calcName);
    parameters->append(calculation);
    auto max = ctxt->conv->convert(mc->arguments->at(4)->expression);
    parameters->append(max);
    return primitive;
}

Util::IJson* ExternConverter_digest::convertExternFunction(
    UNUSED ConversionContext* ctxt, UNUSED const P4::ExternFunction* ef,
    UNUSED const IR::MethodCallExpression* mc, UNUSED const IR::StatOrDecl* s,
    UNUSED const bool emitExterns) {
    if (mc->arguments->size() != 2) {
        modelError("Expected 2 arguments for %1%", mc);
        return nullptr;
    }
    auto primitive = mkPrimitive("generate_digest");
    auto parameters = mkParameters(primitive);
    primitive->emplace_non_null("source_info", mc->sourceInfoJsonObj());
    auto dest = ctxt->conv->convert(mc->arguments->at(0)->expression);
    parameters->append(dest);
    cstring listName = "digest";
    // If we are supplied a type argument that is a named type use
    // that for the list name.
    if (mc->typeArguments->size() == 1) {
        auto typeArg = mc->typeArguments->at(0);
        if (typeArg->is<IR::Type_Name>()) {
            auto origType = ctxt->refMap->getDeclaration(
                typeArg->to<IR::Type_Name>()->path, true);
            if (!origType->is<IR::Type_Struct>()) {
                modelError("%1%: expected a struct type", origType->getNode());
                return nullptr;
            }
            auto st = origType->to<IR::Type_Struct>();
            listName = st->controlPlaneName();
        }
    }
    int id = createFieldList(ctxt, mc->arguments->at(1)->expression, "learn_lists",
                             listName, ctxt->json->learn_lists);
    auto cst = new IR::Constant(id);
    ctxt->typeMap->setType(cst, IR::Type_Bits::get(32));
    auto jcst = ctxt->conv->convert(cst);
    parameters->append(jcst);
    return primitive;
}

Util::IJson* ExternConverter_resubmit::convertExternFunction(
    UNUSED ConversionContext* ctxt, UNUSED const P4::ExternFunction* ef,
    UNUSED const IR::MethodCallExpression* mc, UNUSED const IR::StatOrDecl* s,
    UNUSED const bool emitExterns) {
    if (mc->arguments->size() != 1) {
        modelError("Expected 1 argument for %1%", mc);
        return nullptr;
    }
    auto primitive = mkPrimitive("resubmit");
    auto parameters = mkParameters(primitive);
    primitive->emplace_non_null("source_info", mc->sourceInfoJsonObj());
    cstring listName = "resubmit";
    // If we are supplied a type argument that is a named type use
    // that for the list name.
    if (mc->typeArguments->size() == 1) {
        auto typeArg = mc->typeArguments->at(0);
        if (typeArg->is<IR::Type_Name>()) {
            auto origType = ctxt->refMap->getDeclaration(
                typeArg->to<IR::Type_Name>()->path, true);
            if (!origType->is<IR::Type_Struct>()) {
                modelError("%1%: expected a struct type", origType->getNode());
                return nullptr;
            }
            auto st = origType->to<IR::Type_Struct>();
            listName = st->controlPlaneName();
        }
    }
    int id = createFieldList(ctxt, mc->arguments->at(0)->expression, "field_lists",
                             listName, ctxt->json->field_lists);
    auto cst = new IR::Constant(id);
    ctxt->typeMap->setType(cst, IR::Type_Bits::get(32));
    auto jcst = ctxt->conv->convert(cst);
    parameters->append(jcst);
    return primitive;
}

Util::IJson* ExternConverter_recirculate::convertExternFunction(
    UNUSED ConversionContext* ctxt, UNUSED const P4::ExternFunction* ef,
    UNUSED const IR::MethodCallExpression* mc, UNUSED const IR::StatOrDecl* s,
    UNUSED const bool emitExterns) {
    if (mc->arguments->size() != 1) {
        modelError("Expected 1 argument for %1%", mc);
        return nullptr;
    }
    auto primitive = mkPrimitive("recirculate");
    auto parameters = mkParameters(primitive);
    primitive->emplace_non_null("source_info", mc->sourceInfoJsonObj());
    cstring listName = "recirculate";
    // If we are supplied a type argument that is a named type use
    // that for the list name.
    if (mc->typeArguments->size() == 1) {
        auto typeArg = mc->typeArguments->at(0);
        if (typeArg->is<IR::Type_Name>()) {
            auto origType = ctxt->refMap->getDeclaration(
                typeArg->to<IR::Type_Name>()->path, true);
            if (!origType->is<IR::Type_Struct>()) {
                modelError("%1%: expected a struct type", origType->getNode());
                return nullptr;
            }
            auto st = origType->to<IR::Type_Struct>();
            listName = st->controlPlaneName();
        }
    }
    int id = createFieldList(ctxt, mc->arguments->at(0)->expression, "field_lists",
                             listName, ctxt->json->field_lists);
    auto cst = new IR::Constant(id);
    ctxt->typeMap->setType(cst, IR::Type_Bits::get(32));
    auto jcst = ctxt->conv->convert(cst);
    parameters->append(jcst);
    return primitive;
}

Util::IJson* ExternConverter_mark_to_drop::convertExternFunction(
    UNUSED ConversionContext* ctxt, UNUSED const P4::ExternFunction* ef,
    UNUSED const IR::MethodCallExpression* mc, UNUSED const IR::StatOrDecl* s,
    UNUSED const bool emitExterns) {
    if (mc->arguments->size() != 0) {
        modelError("Expected 0 arguments for %1%", mc);
        return nullptr;
    }
    auto primitive = mkPrimitive("drop");
    (void)mkParameters(primitive);
    primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
    return primitive;
}

Util::IJson* ExternConverter_random::convertExternFunction(
    UNUSED ConversionContext* ctxt, UNUSED const P4::ExternFunction* ef,
    UNUSED const IR::MethodCallExpression* mc, UNUSED const IR::StatOrDecl* s,
    UNUSED const bool emitExterns) {
    if (mc->arguments->size() != 3) {
        modelError("Expected 3 arguments for %1%", mc);
        return nullptr;
    }
    auto primitive =
        mkPrimitive(v1model.random.modify_field_rng_uniform.name);
    auto params = mkParameters(primitive);
    primitive->emplace_non_null("source_info", mc->sourceInfoJsonObj());
    auto dest = ctxt->conv->convert(mc->arguments->at(0)->expression);
    auto lo = ctxt->conv->convert(mc->arguments->at(1)->expression);
    auto hi = ctxt->conv->convert(mc->arguments->at(2)->expression);
    params->append(dest);
    params->append(lo);
    params->append(hi);
    return primitive;
}

Util::IJson* ExternConverter_truncate::convertExternFunction(
    UNUSED ConversionContext* ctxt, UNUSED const P4::ExternFunction* ef,
    UNUSED const IR::MethodCallExpression* mc, UNUSED const IR::StatOrDecl* s,
    UNUSED const bool emitExterns) {
    if (mc->arguments->size() != 1) {
        modelError("Expected 1 arguments for %1%", mc);
        return nullptr;
    }
    auto primitive = mkPrimitive(v1model.truncate.name);
    auto params = mkParameters(primitive);
    primitive->emplace_non_null("source_info", mc->sourceInfoJsonObj());
    auto len = ctxt->conv->convert(mc->arguments->at(0)->expression);
    params->append(len);
    return primitive;
}

Util::IJson* ExternConverter_counter::convertExternObject(
    UNUSED ConversionContext* ctxt, UNUSED const P4::ExternMethod* em,
    UNUSED const IR::MethodCallExpression* mc, UNUSED const IR::StatOrDecl *s,
    UNUSED const bool& emitExterns) {
    if (mc->arguments->size() != 1) {
        modelError("Expected 1 argument for %1%", mc);
        return nullptr;
    }
    auto primitive = mkPrimitive("count");
    auto parameters = mkParameters(primitive);
    primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
    auto ctr = new Util::JsonObject();
    ctr->emplace("type", "counter_array");
    ctr->emplace("value", em->object->controlPlaneName());
    parameters->append(ctr);
    auto index = ctxt->conv->convert(mc->arguments->at(0)->expression);
    parameters->append(index);
    return primitive;
}

void ExternConverter_counter::convertExternInstance(
    UNUSED ConversionContext* ctxt, UNUSED const IR::Declaration* c,
    UNUSED const IR::ExternBlock* eb, UNUSED const bool& emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    auto jctr = new Util::JsonObject();
    jctr->emplace("name", name);
    jctr->emplace("id", nextId("counter_arrays"));
    jctr->emplace_non_null("source_info", eb->sourceInfoJsonObj());
    auto sz = eb->findParameterValue(v1model.counter.sizeParam.name);
    CHECK_NULL(sz);
    if (!sz->is<IR::Constant>()) {
        modelError("%1%: expected a constant", sz->getNode());
        return;
    }
    jctr->emplace("size", sz->to<IR::Constant>()->value);
    jctr->emplace("is_direct", false);
    ctxt->json->counters->append(jctr);
}

Util::IJson* ExternConverter_meter::convertExternObject(
    UNUSED ConversionContext* ctxt, UNUSED const P4::ExternMethod* em,
    UNUSED const IR::MethodCallExpression* mc, UNUSED const IR::StatOrDecl *s,
    UNUSED const bool& emitExterns) {
    if (mc->arguments->size() != 2) {
        modelError("Expected 2 arguments for %1%", mc);
        return nullptr;
    }
    auto primitive = mkPrimitive("execute_meter");
    auto parameters = mkParameters(primitive);
    primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
    auto mtr = new Util::JsonObject();
    mtr->emplace("type", "meter_array");
    mtr->emplace("value", em->object->controlPlaneName());
    parameters->append(mtr);
    auto index = ctxt->conv->convert(mc->arguments->at(0)->expression);
    parameters->append(index);
    auto result = ctxt->conv->convert(mc->arguments->at(1)->expression);
    parameters->append(result);
    return primitive;
}

void ExternConverter_meter::convertExternInstance(
    UNUSED ConversionContext* ctxt, UNUSED const IR::Declaration* c,
    UNUSED const IR::ExternBlock* eb, UNUSED const bool& emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    auto jmtr = new Util::JsonObject();
    jmtr->emplace("name", name);
    jmtr->emplace("id", nextId("meter_arrays"));
    jmtr->emplace_non_null("source_info", eb->sourceInfoJsonObj());
    jmtr->emplace("is_direct", false);
    auto sz = eb->findParameterValue(v1model.meter.sizeParam.name);
    CHECK_NULL(sz);
    if (!sz->is<IR::Constant>()) {
        modelError("%1%: expected a constant", sz->getNode());
        return;
    }
    jmtr->emplace("size", sz->to<IR::Constant>()->value);
    jmtr->emplace("rate_count", 2);
    auto mkind = eb->findParameterValue(v1model.meter.typeParam.name);
    CHECK_NULL(mkind);
    if (!mkind->is<IR::Declaration_ID>()) {
        modelError("%1%: expected a member", mkind->getNode());
        return;
    }
    cstring mkind_name = mkind->to<IR::Declaration_ID>()->name;
    cstring type = "?";
    if (mkind_name == v1model.meter.meterType.packets.name)
        type = "packets";
    else if (mkind_name == v1model.meter.meterType.bytes.name)
        type = "bytes";
    else
        ::error("Unexpected meter type %1%", mkind->getNode());
    jmtr->emplace("type", type);
    ctxt->json->meter_arrays->append(jmtr);
}

Util::IJson* ExternConverter_register::convertExternObject(
    UNUSED ConversionContext* ctxt, UNUSED const P4::ExternMethod* em,
    UNUSED const IR::MethodCallExpression* mc, UNUSED const IR::StatOrDecl *s,
    UNUSED const bool& emitExterns) {
    if (mc->arguments->size() != 2) {
        modelError("Expected 2 arguments for %1%", mc);
        return nullptr;
    }
    auto reg = new Util::JsonObject();
    reg->emplace("type", "register_array");
    cstring name = em->object->controlPlaneName();
    reg->emplace("value", name);
    if (em->method->name == v1model.registers.read.name) {
        auto primitive = mkPrimitive("register_read");
        auto parameters = mkParameters(primitive);
        primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
        auto dest = ctxt->conv->convert(mc->arguments->at(0)->expression);
        parameters->append(dest);
        parameters->append(reg);
        auto index = ctxt->conv->convert(mc->arguments->at(1)->expression);
        parameters->append(index);
        return primitive;
    } else if (em->method->name == v1model.registers.write.name) {
        auto primitive = mkPrimitive("register_write");
        auto parameters = mkParameters(primitive);
        primitive->emplace_non_null("source_info", s->sourceInfoJsonObj());
        parameters->append(reg);
        auto index = ctxt->conv->convert(mc->arguments->at(0)->expression);
        parameters->append(index);
        auto value = ctxt->conv->convert(mc->arguments->at(1)->expression);
        parameters->append(value);
        return primitive;
    }
    return nullptr;
}

void ExternConverter_register::convertExternInstance(
    UNUSED ConversionContext* ctxt, UNUSED const IR::Declaration* c,
    UNUSED const IR::ExternBlock* eb, UNUSED const bool& emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    auto jreg = new Util::JsonObject();
    jreg->emplace("name", name);
    jreg->emplace("id", nextId("register_arrays"));
    jreg->emplace_non_null("source_info", eb->sourceInfoJsonObj());
    auto sz = eb->findParameterValue(v1model.registers.sizeParam.name);
    CHECK_NULL(sz);
    if (!sz->is<IR::Constant>()) {
        modelError("%1%: expected a constant", sz->getNode());
        return;
    }
    if (sz->to<IR::Constant>()->value == 0)
        error("%1%: direct registers are not supported in bmv2", inst);
    jreg->emplace("size", sz->to<IR::Constant>()->value);
    if (!eb->instanceType->is<IR::Type_SpecializedCanonical>()) {
        modelError("%1%: Expected a generic specialized type", eb->instanceType);
        return;
    }
    auto st = eb->instanceType->to<IR::Type_SpecializedCanonical>();
    if (st->arguments->size() != 1) {
        modelError("%1%: expected 1 type argument", st);
        return;
    }
    auto regType = st->arguments->at(0);
    if (!regType->is<IR::Type_Bits>()) {
        ::error("%1%: Only registers with bit or int types are currently supported", eb);
        return;
    }
    unsigned width = regType->width_bits();
    if (width == 0) {
        ::error("%1%: unknown width", st->arguments->at(0));
        return;
    }
    jreg->emplace("bitwidth", width);
    ctxt->json->register_arrays->append(jreg);
}

Util::IJson* ExternConverter_direct_counter::convertExternObject(
    UNUSED ConversionContext* ctxt, UNUSED const P4::ExternMethod* em,
    UNUSED const IR::MethodCallExpression* mc, UNUSED const IR::StatOrDecl *s,
    UNUSED const bool& emitExterns) {
    if (mc->arguments->size() != 0) {
        modelError("Expected 0 argument for %1%", mc);
        return nullptr;
    }
    // Do not generate any code for this operation
    return nullptr;
}

void ExternConverter_direct_counter::convertExternInstance(
    UNUSED ConversionContext* ctxt, UNUSED const IR::Declaration* c,
    UNUSED const IR::ExternBlock* eb, UNUSED const bool& emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    auto it = ctxt->structure->directCounterMap.find(name);
    if (it == ctxt->structure->directCounterMap.end()) {
        ::warning(ErrorType::WARN_UNUSED, "%1%: Direct counter not used; ignoring", inst);
    } else {
        auto jctr = new Util::JsonObject();
        jctr->emplace("name", name);
        jctr->emplace("id", nextId("counter_arrays"));
        jctr->emplace("is_direct", true);
        jctr->emplace("binding", it->second->controlPlaneName());
        jctr->emplace_non_null("source_info", eb->sourceInfoJsonObj());
        ctxt->json->counters->append(jctr);
    }
}

Util::IJson* ExternConverter_direct_meter::convertExternObject(
    UNUSED ConversionContext* ctxt, UNUSED const P4::ExternMethod* em,
    UNUSED const IR::MethodCallExpression* mc, UNUSED const IR::StatOrDecl *s,
    UNUSED const bool& emitExterns) {
    if (mc->arguments->size() != 1) {
        modelError("Expected 1 argument for %1%", mc);
        return nullptr;
    }
    auto dest = mc->arguments->at(0);
    ctxt->structure->directMeterMap.setDestination(em->object, dest->expression);
    // Do not generate any code for this operation
    return nullptr;
}

void ExternConverter_direct_meter::convertExternInstance(
    UNUSED ConversionContext* ctxt, UNUSED const IR::Declaration* c,
    UNUSED const IR::ExternBlock* eb, UNUSED const bool& emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    auto info = ctxt->structure->directMeterMap.getInfo(c);
    CHECK_NULL(info);
    CHECK_NULL(info->table);
    CHECK_NULL(info->destinationField);

    auto jmtr = new Util::JsonObject();
    jmtr->emplace("name", name);
    jmtr->emplace("id", nextId("meter_arrays"));
    jmtr->emplace_non_null("source_info", eb->sourceInfoJsonObj());
    jmtr->emplace("is_direct", true);
    jmtr->emplace("rate_count", 2);
    auto mkind = eb->findParameterValue(v1model.directMeter.typeParam.name);
    CHECK_NULL(mkind);
    if (!mkind->is<IR::Declaration_ID>()) {
        modelError("%1%: expected a member", mkind->getNode());
        return;
    }
    cstring mkind_name = mkind->to<IR::Declaration_ID>()->name;
    cstring type = "?";
    if (mkind_name == v1model.meter.meterType.packets.name) {
        type = "packets";
    } else if (mkind_name == v1model.meter.meterType.bytes.name) {
        type = "bytes";
    } else {
        modelError("%1%: unexpected meter type", mkind->getNode());
        return;
    }
    jmtr->emplace("type", type);
    jmtr->emplace("size", info->tableSize);
    cstring tblname = info->table->controlPlaneName();
    jmtr->emplace("binding", tblname);
    auto result = ctxt->conv->convert(info->destinationField);
    jmtr->emplace("result_target", result->to<Util::JsonObject>()->get("value"));
    ctxt->json->meter_arrays->append(jmtr);
}

void ExternConverter_action_profile::convertExternInstance(
    UNUSED ConversionContext* ctxt, UNUSED const IR::Declaration* c,
    UNUSED const IR::ExternBlock* eb, UNUSED const bool& emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    // Might call this multiple times if the selector/profile is used more than
    // once in a pipeline, so only add it to the action_profiles once
    if (BMV2::JsonObjects::find_object_by_name(ctxt->action_profiles, name))
        return;
    auto action_profile = new Util::JsonObject();
    action_profile->emplace("name", name);
    action_profile->emplace("id", nextId("action_profiles"));
    action_profile->emplace_non_null("source_info", eb->sourceInfoJsonObj());

    auto add_size = [&action_profile, &eb](const cstring &pname) {
        auto sz = eb->findParameterValue(pname);
        if (!sz->is<IR::Constant>()) {
            ::error("%1%: expected a constant", sz);
            return;
        }
        action_profile->emplace("max_size", sz->to<IR::Constant>()->value);
    };

    if (eb->type->name == v1model.action_profile.name) {
        add_size(v1model.action_profile.sizeParam.name);
    } else {
        add_size(v1model.action_selector.sizeParam.name);
        auto selector = new Util::JsonObject();
        auto hash = eb->findParameterValue(
            v1model.action_selector.algorithmParam.name);
        if (!hash->is<IR::Declaration_ID>()) {
            modelError("%1%: expected a member", hash->getNode());
            return;
        }
        auto algo = ExternConverter::convertHashAlgorithm(hash->to<IR::Declaration_ID>()->name);
        selector->emplace("algo", algo);
        auto input = ctxt->selector_check->get_selector_input(inst);
        if (input == nullptr) {
            // the selector is never used by any table, we cannot figure out its
            // input and therefore cannot include it in the JSON
            ::warning("Action selector '%1%' is never referenced by a table "
                      "and cannot be included in bmv2 JSON", c);
            return;
        }
        auto j_input = mkArrayField(selector, "input");
        for (auto expr : *input) {
            auto jk = ctxt->conv->convert(expr);
            j_input->append(jk);
        }
        action_profile->emplace("selector", selector);
    }

    ctxt->action_profiles->append(action_profile);
}

// action selector conversion is the same as action profile
void ExternConverter_action_selector::convertExternInstance(
    UNUSED ConversionContext* ctxt, UNUSED const IR::Declaration* c,
    UNUSED const IR::ExternBlock* eb, UNUSED const bool& emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    // Might call this multiple times if the selector/profile is used more than
    // once in a pipeline, so only add it to the action_profiles once
    if (BMV2::JsonObjects::find_object_by_name(ctxt->action_profiles, name))
        return;
    auto action_profile = new Util::JsonObject();
    action_profile->emplace("name", name);
    action_profile->emplace("id", nextId("action_profiles"));
    action_profile->emplace_non_null("source_info", eb->sourceInfoJsonObj());

    auto add_size = [&action_profile, &eb](const cstring &pname) {
        auto sz = eb->findParameterValue(pname);
        if (!sz->is<IR::Constant>()) {
            ::error("%1%: expected a constant", sz);
            return;
        }
        action_profile->emplace("max_size", sz->to<IR::Constant>()->value);
    };

    if (eb->type->name == v1model.action_profile.name) {
        add_size(v1model.action_profile.sizeParam.name);
    } else {
        add_size(v1model.action_selector.sizeParam.name);
        auto selector = new Util::JsonObject();
        auto hash = eb->findParameterValue(
            v1model.action_selector.algorithmParam.name);
        if (!hash->is<IR::Declaration_ID>()) {
            modelError("%1%: expected a member", hash->getNode());
            return;
        }
        auto algo = ExternConverter::convertHashAlgorithm(hash->to<IR::Declaration_ID>()->name);
        selector->emplace("algo", algo);
        auto input = ctxt->selector_check->get_selector_input(inst);
        if (input == nullptr) {
            // the selector is never used by any table, we cannot figure out its
            // input and therefore cannot include it in the JSON
            ::warning("Action selector '%1%' is never referenced by a table "
                      "and cannot be included in bmv2 JSON", c);
            return;
        }
        auto j_input = mkArrayField(selector, "input");
        for (auto expr : *input) {
            auto jk = ctxt->conv->convert(expr);
            j_input->append(jk);
        }
        action_profile->emplace("selector", selector);
    }

    ctxt->action_profiles->append(action_profile);
}

void
SimpleSwitchBackend::modelError(const char* format, const IR::Node* node) const {
    ::error(format, node);
    ::error("Are you using an up-to-date v1model.p4?");
}

cstring
SimpleSwitchBackend::createCalculation(cstring algo, const IR::Expression* fields,
                                       Util::JsonArray* calculations, bool withPayload,
                                       const IR::Node* sourcePositionNode = nullptr) {
    cstring calcName = refMap->newName("calc_");
    auto calc = new Util::JsonObject();
    calc->emplace("name", calcName);
    calc->emplace("id", nextId("calculations"));
    if (sourcePositionNode != nullptr)
        calc->emplace_non_null("source_info", sourcePositionNode->sourceInfoJsonObj());
    calc->emplace("algo", algo);
    if (!fields->is<IR::ListExpression>()) {
        // expand it into a list
        auto list = new IR::ListExpression({});
        auto type = typeMap->getType(fields, true);
        if (!type->is<IR::Type_StructLike>()) {
            modelError("%1%: expected a struct", fields);
            return calcName;
        }
        for (auto f : type->to<IR::Type_StructLike>()->fields) {
            auto e = new IR::Member(fields, f->name);
            auto ftype = typeMap->getType(f);
            typeMap->setType(e, ftype);
            list->push_back(e);
        }
        fields = list;
        typeMap->setType(fields, type);
    }
    auto jright = conv->convertWithConstantWidths(fields);
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

void
SimpleSwitchBackend::convertChecksum(const IR::BlockStatement *block, Util::JsonArray* checksums,
                                     Util::JsonArray* calculations, bool verify) {
    if (errorCount() > 0)
        return;
    for (auto stat : block->components) {
        if (auto blk = stat->to<IR::BlockStatement>()) {
            convertChecksum(blk, checksums, calculations, verify);
            continue;
        } else if (auto mc = stat->to<IR::MethodCallStatement>()) {
            auto mi = P4::MethodInstance::resolve(mc, refMap, typeMap);
            if (auto em = mi->to<P4::ExternFunction>()) {
                cstring functionName = em->method->name.name;
                if ((verify && (functionName == v1model.verify_checksum.name ||
                                functionName == v1model.verify_checksum_with_payload.name)) ||
                    (!verify && (functionName == v1model.update_checksum.name ||
                                 functionName == v1model.update_checksum_with_payload.name))) {
                    bool usePayload = functionName.endsWith("_with_payload");
                    if (mi->expr->arguments->size() != 4) {
                        modelError("%1%: Expected 4 arguments", mc);
                        return;
                    }
                    auto cksum = new Util::JsonObject();
                    auto ei = P4::EnumInstance::resolve(
                        mi->expr->arguments->at(3)->expression, typeMap);
                    cstring algo = ExternConverter::convertHashAlgorithm(ei->name);
                    cstring calcName = createCalculation(
                        algo, mi->expr->arguments->at(1)->expression,
                        calculations, usePayload, mc);
                    cksum->emplace("name", refMap->newName("cksum_"));
                    cksum->emplace("id", nextId("checksums"));
                    cksum->emplace_non_null("source_info", stat->sourceInfoJsonObj());
                    auto jleft = conv->convert(mi->expr->arguments->at(2)->expression);
                    cksum->emplace("target", jleft->to<Util::JsonObject>()->get("value"));
                    cksum->emplace("type", "generic");
                    cksum->emplace("calculation", calcName);
                    cksum->emplace("verify", verify);
                    cksum->emplace("update", !verify);
                    auto ifcond = conv->convert(
                        mi->expr->arguments->at(0)->expression, true, false);
                    cksum->emplace("if_cond", ifcond);
                    checksums->append(cksum);
                    continue;
                }
            }
        }
        ::error("%1%: Only calls to %2% or %3% allowed", stat,
                verify ? v1model.verify_checksum.name : v1model.update_checksum.name,
                verify ? v1model.verify_checksum_with_payload.name :
                v1model.update_checksum_with_payload.name);
    }
}

void SimpleSwitchBackend::createActions(ConversionContext* ctxt, V1ProgramStructure* structure) {
    auto cvt = new ActionConverter(ctxt, options.emitExterns);
    for (auto it : structure->actions) {
        auto action = it.first;
        action->apply(*cvt);
    }
}

void
SimpleSwitchBackend::convert(const IR::ToplevelBlock* tlb) {
    structure = new V1ProgramStructure();

    auto parseV1Arch = new ParseV1Architecture(structure);
    auto main = tlb->getMain();
    if (!main) return;  // no main

    if (main->type->name != "V1Switch")
        ::warning(ErrorType::WARN_INVALID, "%1%: the main package should be called V1Switch"
                  "; are you using the wrong architecture?", main->type->name);

    main->apply(*parseV1Arch);
    if (::errorCount() > 0)
        return;

    /// Declaration which introduces the user metadata.
    /// We expect this to be a struct type.
    const IR::Type_Struct* userMetaType = nullptr;
    cstring userMetaName = refMap->newName("userMetadata");

    // Find the user metadata declaration
    auto parser = main->findParameterValue(v1model.sw.parser.name);
    if (parser == nullptr) return;
    if (!parser->is<IR::ParserBlock>()) {
        modelError("%1%: main package  match the expected model", main);
        return;
    }
    auto params = parser->to<IR::ParserBlock>()->container->getApplyParameters();
    BUG_CHECK(params->size() == 4, "%1%: expected 4 parameters", parser);
    auto metaParam = params->parameters.at(2);
    auto paramType = metaParam->type;
    if (!paramType->is<IR::Type_Name>()) {
        ::error("%1%: expected the user metadata type to be a struct", paramType);
        return;
    }
    auto decl = refMap->getDeclaration(paramType->to<IR::Type_Name>()->path);
    if (!decl->is<IR::Type_Struct>()) {
        ::error("%1%: expected the user metadata type to be a struct", paramType);
        return;
    }
    userMetaType = decl->to<IR::Type_Struct>();
    LOG2("User metadata type is " << userMetaType);

    {
        // Find the header types
        auto headersParam = params->parameters.at(1);
        auto headersType = headersParam->type;
        if (!headersType->is<IR::Type_Name>()) {
            ::error("%1%: expected type to be a struct", headersParam->type);
            return;
        }
        decl = refMap->getDeclaration(headersType->to<IR::Type_Name>()->path);
        auto st = decl->to<IR::Type_Struct>();
        if (st == nullptr) {
            ::error("%1%: expected type to be a struct", headersParam->type);
            return;
        }
        LOG2("Headers type is " << st);
        for (auto f : st->fields) {
            auto t = typeMap->getType(f, true);
            if (!t->is<IR::Type_Header>() &&
                !t->is<IR::Type_Stack>() &&
                !t->is<IR::Type_HeaderUnion>()) {
                ::error("%1%: the type should be a struct of headers, stacks, or unions",
                        headersParam->type);
                return;
            }
        }
    }

    auto evaluator = new P4::EvaluatorPass(refMap, typeMap);
    auto program = tlb->getProgram();
    // These passes are logically bmv2-specific
    PassManager simplify = {
        new ParseAnnotations(),
        new RenameUserMetadata(refMap, userMetaType, userMetaName),
        new P4::ClearTypeMap(typeMap),  // because the user metadata type has changed
        new P4::SynthesizeActions(refMap, typeMap,
                                  new SkipControls(&structure->non_pipeline_controls)),
        new P4::MoveActionsToTables(refMap, typeMap),
        new P4::TypeChecking(refMap, typeMap),
        new P4::SimplifyControlFlow(refMap, typeMap),
        new LowerExpressions(typeMap),
        new P4::ConstantFolding(refMap, typeMap, false),
        new P4::TypeChecking(refMap, typeMap),
        new RemoveComplexExpressions(refMap, typeMap,
                                     new ProcessControls(&structure->pipeline_controls)),
        new P4::SimplifyControlFlow(refMap, typeMap),
        new P4::RemoveAllUnusedDeclarations(refMap),
        evaluator,
        new VisitFunctor([this, evaluator]() { toplevel = evaluator->getToplevelBlock(); }),
    };

    auto hook = options.getDebugHook();
    simplify.addDebugHook(hook);

    program->apply(simplify);

    // map IR node to compile-time allocated resource blocks.
    toplevel->apply(*new BMV2::BuildResourceMap(&structure->resourceMap));

    // field list and learn list ids in bmv2 are not consistent with ids for
    // other objects: they need to start at 1 (not 0) since the id is also used
    // as a "flag" to indicate that a certain simple_switch primitive has been
    // called (e.g. resubmit or generate_digest)
    BMV2::nextId("field_lists");
    BMV2::nextId("learn_lists");
    json->add_program_info(options.file);
    json->add_meta_info();

    // convert all enums to json
    for (const auto &pEnum : *enumMap) {
        auto name = pEnum.first->getName();
        for (const auto &pEntry : *pEnum.second) {
            json->add_enum(name, pEntry.first, pEntry.second);
        }
    }
    if (::errorCount() > 0)
        return;

    main = toplevel->getMain();
    if (!main) return;  // no main
    main->apply(*parseV1Arch);
    PassManager updateStructure {
        new DiscoverV1Structure(structure),
    };
    program = toplevel->getProgram();
    program->apply(updateStructure);

    /// generate error types
    for (const auto &p : structure->errorCodesMap) {
        auto name = p.first->toString();
        auto type = p.second;
        json->add_error(name, type);
    }

    cstring scalarsName = refMap->newName("scalars");
    // This visitor is used in multiple passes to convert expression to json
    conv = new SimpleSwitchExpressionConverter(refMap, typeMap, structure, scalarsName);

    auto ctxt = new ConversionContext(refMap, typeMap, toplevel, structure, conv, json);

    auto hconv = new HeaderConverter(ctxt, scalarsName);
    program->apply(*hconv);

    auto pconv = new ParserConverter(ctxt);
    structure->parser->apply(*pconv);

    createActions(ctxt, structure);

    auto cconv = new ControlConverter(ctxt, "ingress", options.emitExterns);
    structure->ingress->apply(*cconv);

    cconv = new ControlConverter(ctxt, "egress", options.emitExterns);
    structure->egress->apply(*cconv);

    auto dconv = new DeparserConverter(ctxt);
    structure->deparser->apply(*dconv);

    convertChecksum(structure->compute_checksum->body, json->checksums,
                    json->calculations, false);

    convertChecksum(structure->verify_checksum->body, json->checksums,
                    json->calculations, true);

    (void)toplevel->apply(ConvertGlobals(ctxt, options.emitExterns));
}

}  // namespace BMV2
