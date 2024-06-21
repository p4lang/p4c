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

#include "psaSwitch.h"

#include "frontends/common/model.h"
#include "frontends/p4/cloner.h"

namespace BMV2 {

using namespace P4::literals;

void PsaCodeGenerator::create(ConversionContext *ctxt) {
    createTypes(ctxt);
    createHeaders(ctxt);
    createScalars(ctxt);
    createExterns();
    createParsers(ctxt);
    createActions(ctxt);
    createControls(ctxt);
    createDeparsers(ctxt);
    createGlobals();
}

void PsaCodeGenerator::createStructLike(ConversionContext *ctxt, const IR::Type_StructLike *st) {
    CHECK_NULL(st);
    cstring name = st->controlPlaneName();
    unsigned max_length = 0;  // for variable-sized headers
    bool varbitFound = false;
    auto fields = new Util::JsonArray();
    LOG5("In createStructLike with struct " << st->toString());
    for (auto f : st->fields) {
        auto field = new Util::JsonArray();
        auto ftype = typeMap->getType(f, true);
        LOG5("Iterating field with field " << f << " and type " << ftype->toString());
        if (ftype->to<IR::Type_StructLike>()) {
            BUG("%1%: nested structure", st);
        } else if (ftype->is<IR::Type_Boolean>()) {
            field->append(f->name.name);
            field->append(1);
            field->append(false);
            max_length += 1;
        } else if (auto type = ftype->to<IR::Type_Bits>()) {
            field->append(f->name.name);
            field->append(type->size);
            field->append(type->isSigned);
            max_length += type->size;
        } else if (auto type = ftype->to<IR::Type_Varbits>()) {
            field->append(f->name.name);
            max_length += type->size;
            field->append("*");
            if (varbitFound)
                ::error(ErrorType::ERR_UNSUPPORTED,
                        "%1%: headers with multiple varbit fields are not supported", st);
            varbitFound = true;
        } else if (ftype->is<IR::Type_Error>()) {
            field->append(f->name.name);
            field->append(error_width);
            field->append(false);
            max_length += error_width;
        } else if (ftype->to<IR::Type_Stack>()) {
            BUG("%1%: nested stack", st);
        } else {
            BUG("%1%: unexpected type for %2%.%3%", ftype, st, f->name);
        }
        fields->append(field);
    }
    // must add padding
    unsigned padding = max_length % 8;
    if (padding != 0) {
        cstring name = refMap->newName("_padding");
        auto field = new Util::JsonArray();
        field->append(name);
        field->append(8 - padding);
        field->append(false);
        fields->append(field);
    }

    unsigned max_length_bytes = (max_length + padding) / 8;
    if (!varbitFound) {
        // ignore
        max_length = 0;
        max_length_bytes = 0;
    }
    ctxt->json->add_header_type(name, fields, max_length_bytes);
}

void PsaCodeGenerator::createTypes(ConversionContext *ctxt) {
    for (auto kv : header_types) createStructLike(ctxt, kv.second);
    for (auto kv : metadata_types) createStructLike(ctxt, kv.second);
    for (auto kv : header_union_types) {
        auto st = kv.second;
        auto fields = new Util::JsonArray();
        for (auto f : st->fields) {
            auto field = new Util::JsonArray();
            auto ftype = typeMap->getType(f, true);
            auto ht = ftype->to<IR::Type_Header>();
            CHECK_NULL(ht);
            field->append(f->name.name);
            field->append(ht->name.name);
        }
        ctxt->json->add_union_type(st->name, fields);
    }
    /* TODO */
    // add errors to json
    // add enums to json
}

void PsaCodeGenerator::createScalars(ConversionContext *ctxt) {
    auto name = scalars.begin()->first;
    ctxt->json->add_header("scalars_t"_cs, name);
    ctxt->json->add_header_type("scalars_t"_cs);
    unsigned max_length = 0;
    for (auto kv : scalars) {
        LOG5("Adding a scalar field " << kv.second << " to generated json");
        auto field = new Util::JsonArray();
        auto ftype = typeMap->getType(kv.second, true);
        if (auto type = ftype->to<IR::Type_Bits>()) {
            field->append(kv.second->name);
            max_length += type->size;
            field->append(type->size);
            field->append(type->isSigned);
        } else if (ftype->is<IR::Type_Boolean>()) {
            field->append(kv.second->name);
            max_length += 1;
            field->append(1);
            field->append(false);
        } else {
            BUG_CHECK(kv.second, "%1 is not of Type_Bits or Type_Boolean");
        }
        ctxt->json->add_header_field("scalars_t"_cs, field);
    }
    // must add padding
    unsigned padding = max_length % 8;
    if (padding != 0) {
        cstring name = refMap->newName("_padding");
        auto field = new Util::JsonArray();
        field->append(name);
        field->append(8 - padding);
        field->append(false);
        ctxt->json->add_header_field("scalars_t"_cs, field);
    }
}

void PsaCodeGenerator::createHeaders(ConversionContext *ctxt) {
    for (auto kv : headers) {
        auto type = kv.second->type->to<IR::Type_StructLike>();
        ctxt->json->add_header(type->controlPlaneName(), kv.second->name);
    }
    for (auto kv : metadata) {
        auto type = kv.second->type->to<IR::Type_StructLike>();
        ctxt->json->add_metadata(type->controlPlaneName(), kv.second->name);
    }
    /* TODO */
    // for (auto kv : header_stacks) {
    //     json->add_header_stack(stack_type, stack_name, stack_size, ids);
    // }
    for (auto kv : header_unions) {
        auto header_name = kv.first;
        auto header_type = kv.second->to<IR::Type_StructLike>()->controlPlaneName();
        // We have to add separately a header instance for all
        // headers in the union.  Each instance will be named with
        // a prefix including the union name, e.g., "u.h"
        Util::JsonArray *fields = new Util::JsonArray();
        for (auto uf : kv.second->to<IR::Type_HeaderUnion>()->fields) {
            auto uft = typeMap->getType(uf, true);
            auto h_name = header_name + "." + uf->controlPlaneName();
            auto h_type = uft->to<IR::Type_StructLike>()->controlPlaneName();
            unsigned id = ctxt->json->add_header(h_type, h_name);
            fields->append(id);
        }
        ctxt->json->add_union(header_type, fields, header_name);
    }
}

void PsaCodeGenerator::createParsers(ConversionContext *ctxt) {
    {
        auto cvt = new ParserConverter(ctxt, "ingress_parser"_cs);
        auto ingress = parsers.at("ingress"_cs);
        ingress->apply(*cvt);
    }
    {
        auto cvt = new ParserConverter(ctxt, "egress_parser"_cs);
        auto ingress = parsers.at("egress"_cs);
        ingress->apply(*cvt);
    }
}

void PsaCodeGenerator::createExterns() {
    /* TODO */
    // add parse_vsets to json
    // add meter_arrays to json
    // add counter_arrays to json
    // add register_arrays to json
    // add checksums to json
    // add learn_list to json
    // add calculations to json
    // add extern_instances to json
}

void PsaCodeGenerator::createActions(ConversionContext *ctxt) {
    auto cvt = new ActionConverter(ctxt, true);
    for (auto it : actions) {
        auto action = it.first;
        action->apply(*cvt);
    }
}

void PsaCodeGenerator::createControls(ConversionContext *ctxt) {
    auto cvt = new BMV2::ControlConverter<Standard::Arch::PSA>(ctxt, "ingress"_cs, true);
    auto ingress = pipelines.at("ingress"_cs);
    ingress->apply(*cvt);

    cvt = new BMV2::ControlConverter<Standard::Arch::PSA>(ctxt, "egress"_cs, true);
    auto egress = pipelines.at("egress"_cs);
    egress->apply(*cvt);
}

void PsaCodeGenerator::createDeparsers(ConversionContext *ctxt) {
    {
        auto cvt = new DeparserConverter(ctxt, "ingress_deparser"_cs);
        auto ingress = deparsers.at("ingress"_cs);
        ingress->apply(*cvt);
    }
    {
        auto cvt = new DeparserConverter(ctxt, "egress_deparser"_cs);
        auto egress = deparsers.at("egress"_cs);
        egress->apply(*cvt);
    }
}

void PsaCodeGenerator::createGlobals() {
    /* TODO */
    // for (auto e : globals) {
    //     convertExternInstances(e->node->to<IR::Declaration>(), e->to<IR::ExternBlock>());
    // }
}

void PsaSwitchBackend::convert(const IR::ToplevelBlock *tlb) {
    CHECK_NULL(tlb);
    PsaCodeGenerator structure(refMap, typeMap);

    auto parsePsaArch = new ParsePsaArchitecture(&structure);
    auto main = tlb->getMain();
    if (!main) return;

    if (main->type->name != "PSA_Switch")
        ::warning(ErrorType::WARN_INVALID,
                  "%1%: the main package should be called PSA_Switch"
                  "; are you using the wrong architecture?",
                  main->type->name);

    main->apply(*parsePsaArch);

    auto evaluator = new P4::EvaluatorPass(refMap, typeMap);
    auto program = tlb->getProgram();
    PassManager simplify = {
        /* TODO */
        // new RenameUserMetadata(refMap, userMetaType, userMetaName),
        new P4::ClearTypeMap(typeMap),  // because the user metadata type has changed
        new P4::SynthesizeActions(refMap, typeMap,
                                  new SkipControls(&structure.non_pipeline_controls)),
        new P4::MoveActionsToTables(refMap, typeMap),
        new P4::TypeChecking(refMap, typeMap),
        new P4::SimplifyControlFlow(refMap, typeMap),
        new LowerExpressions(typeMap),
        new PassRepeated(
            {new P4::ConstantFolding(refMap, typeMap), new P4::StrengthReduction(refMap, typeMap)}),
        new P4::TypeChecking(refMap, typeMap),
        new P4::RemoveComplexExpressions(refMap, typeMap,
                                         new ProcessControls(&structure.pipeline_controls)),
        new P4::SimplifyControlFlow(refMap, typeMap),
        new P4::RemoveAllUnusedDeclarations(refMap, P4::RemoveUnusedPolicy()),
        // Converts the DAG into a TREE (at least for expressions)
        // This is important later for conversion to JSON.
        new P4::CloneExpressions(),
        new P4::ClearTypeMap(typeMap),
        evaluator,
        [this, evaluator, structure]() { toplevel = evaluator->getToplevelBlock(); },
    };
    auto hook = options.getDebugHook();
    simplify.addDebugHook(hook);
    program->apply(simplify);

    // map IR node to compile-time allocated resource blocks.
    toplevel->apply(*new BMV2::BuildResourceMap(&structure.resourceMap));

    main = toplevel->getMain();
    if (!main) return;  // no main
    main->apply(*parsePsaArch);
    if (::errorCount() > 0) return;
    program = toplevel->getProgram();

    PassManager toJson = {new DiscoverStructure(&structure),
                          new InspectPsaProgram(refMap, typeMap, &structure),
                          new ConvertPsaToJson(refMap, typeMap, toplevel, json, &structure)};
    for (const auto &pEnum : *enumMap) {
        auto name = pEnum.first->getName();
        for (const auto &pEntry : *pEnum.second) {
            json->add_enum(name, pEntry.first, pEntry.second);
        }
    }
    program->apply(toJson);
    json->add_program_info(cstring(options.file));
    json->add_meta_info();
}

cstring PsaCodeGenerator::convertHashAlgorithm(cstring algo) {
    if (algo == "CRC16") {
        return "crc16"_cs;
    }
    if (algo == "CRC16_CUSTOM") {
        return "crc16_custom"_cs;
    }
    if (algo == "CRC32") {
        return "crc32"_cs;
    }
    if (algo == "CRC32_CUSTOM") {
        return "crc32_custom"_cs;
    }
    if (algo == "IDENTITY") {
        return "identity"_cs;
    }

    return nullptr;
}

ExternConverter_Hash ExternConverter_Hash::singleton;
ExternConverter_Checksum ExternConverter_Checksum::singleton;
ExternConverter_InternetChecksum ExternConverter_InternetChecksum::singleton;
ExternConverter_Counter ExternConverter_Counter::singleton;
ExternConverter_DirectCounter ExternConverter_DirectCounter::singleton;
ExternConverter_Meter ExternConverter_Meter::singleton;
ExternConverter_DirectMeter ExternConverter_DirectMeter::singleton;
ExternConverter_Register ExternConverter_Register::singleton;
ExternConverter_Random ExternConverter_Random::singleton;
ExternConverter_ActionProfile ExternConverter_ActionProfile::singleton;
ExternConverter_ActionSelector ExternConverter_ActionSelector::singleton;
ExternConverter_Digest ExternConverter_Digest::singleton;

Util::IJson *ExternConverter_Hash::convertExternObject(UNUSED ConversionContext *ctxt,
                                                       const P4::ExternMethod *em,
                                                       const IR::MethodCallExpression *mc,
                                                       UNUSED const IR::StatOrDecl *s,
                                                       UNUSED const bool &emitExterns) {
    Util::JsonObject *primitive = nullptr;
    if (mc->arguments->size() == 2)
        primitive = mkPrimitive("_" + em->originalExternType->name + "_" + em->method->name);
    else if (mc->arguments->size() == 4)
        primitive = mkPrimitive("_" + em->originalExternType->name + "_" + "get_hash_mod");
    else {
        modelError("Expected 1 or 3 arguments for %1%", mc);
        return nullptr;
    }
    auto parameters = mkParameters(primitive);
    primitive->emplace_non_null("source_info"_cs, s->sourceInfoJsonObj());
    auto hash = new Util::JsonObject();
    hash->emplace("type"_cs, "extern");
    hash->emplace("value"_cs, em->object->controlPlaneName());
    parameters->append(hash);
    if (mc->arguments->size() == 2) {  // get_hash
        auto dst = ctxt->conv->convertLeftValue(mc->arguments->at(0)->expression);
        auto fieldList = new Util::JsonObject();
        fieldList->emplace("type"_cs, "field_list");
        auto fieldsJson = ctxt->conv->convert(mc->arguments->at(1)->expression, true, false);
        fieldList->emplace("value"_cs, fieldsJson);
        parameters->append(dst);
        parameters->append(fieldList);
    } else {  // get_hash with base and mod
        auto dst = ctxt->conv->convertLeftValue(mc->arguments->at(0)->expression);
        auto base = ctxt->conv->convert(mc->arguments->at(1)->expression);
        auto fieldList = new Util::JsonObject();
        fieldList->emplace("type"_cs, "field_list");
        auto fieldsJson = ctxt->conv->convert(mc->arguments->at(2)->expression, true, false);
        fieldList->emplace("value"_cs, fieldsJson);
        auto max = ctxt->conv->convert(mc->arguments->at(3)->expression);
        parameters->append(dst);
        parameters->append(base);
        parameters->append(fieldList);
        parameters->append(max);
    }
    return primitive;
}

Util::IJson *ExternConverter_Checksum::convertExternObject(
    UNUSED ConversionContext *ctxt, UNUSED const P4::ExternMethod *em,
    UNUSED const IR::MethodCallExpression *mc, UNUSED const IR::StatOrDecl *s,
    UNUSED const bool &emitExterns) {
    auto primitive = mkPrimitive("Checksum"_cs);
    return primitive;
}

Util::IJson *ExternConverter_InternetChecksum::convertExternObject(
    UNUSED ConversionContext *ctxt, UNUSED const P4::ExternMethod *em,
    UNUSED const IR::MethodCallExpression *mc, UNUSED const IR::StatOrDecl *s,
    UNUSED const bool &emitExterns) {
    Util::JsonObject *primitive = nullptr;
    if (em->method->name == "add" || em->method->name == "subtract" ||
        em->method->name == "get_state" || em->method->name == "set_state") {
        if (mc->arguments->size() != 1) {
            modelError("Expected 1 argument for %1%", mc);
            return nullptr;
        } else
            primitive = mkPrimitive("_" + em->originalExternType->name + "_" + em->method->name);
    } else if (em->method->name == "get") {
        if (mc->arguments->size() == 1)
            primitive = mkPrimitive("_" + em->originalExternType->name + "_" + em->method->name);
        else if (mc->arguments->size() == 2)
            primitive = mkPrimitive("_" + em->originalExternType->name + "_" + "get_verify");
        else {
            modelError("Unexpected number of arguments for %1%", mc);
            return nullptr;
        }
    } else if (em->method->name == "clear") {
        if (mc->arguments->size() != 0) {
            modelError("Expected 0 argument for %1%", mc);
            return nullptr;
        } else
            primitive = mkPrimitive("_" + em->originalExternType->name + "_" + em->method->name);
    }
    auto parameters = mkParameters(primitive);
    primitive->emplace_non_null("source_info"_cs, s->sourceInfoJsonObj());
    auto cksum = new Util::JsonObject();
    cksum->emplace("type"_cs, "extern");
    cksum->emplace("value"_cs, em->object->controlPlaneName());
    parameters->append(cksum);
    if (em->method->name == "add" || em->method->name == "subtract") {
        auto fieldList = new Util::JsonObject();
        fieldList->emplace("type"_cs, "field_list");
        auto fieldsJson = ctxt->conv->convert(mc->arguments->at(0)->expression, true, false);
        fieldList->emplace("value"_cs, fieldsJson);
        parameters->append(fieldList);
    } else if (em->method->name != "clear") {
        if (mc->arguments->size() == 2) {  // get_verify
            auto dst = ctxt->conv->convertLeftValue(mc->arguments->at(0)->expression);
            auto equOp = ctxt->conv->convert(mc->arguments->at(1)->expression);
            parameters->append(dst);
            parameters->append(equOp);
        } else if (mc->arguments->size() == 1) {  // get or get_state or set_state
            auto dst = ctxt->conv->convert(mc->arguments->at(0)->expression);
            parameters->append(dst);
        }
    }
    return primitive;
}

Util::IJson *ExternConverter_Counter::convertExternObject(UNUSED ConversionContext *ctxt,
                                                          UNUSED const P4::ExternMethod *em,
                                                          UNUSED const IR::MethodCallExpression *mc,
                                                          UNUSED const IR::StatOrDecl *s,
                                                          UNUSED const bool &emitExterns) {
    if (mc->arguments->size() != 1) {
        modelError("Expected 1 argument for %1%", mc);
        return nullptr;
    }
    auto primitive = mkPrimitive("_" + em->originalExternType->name + "_" + em->method->name);
    auto parameters = mkParameters(primitive);
    primitive->emplace_non_null("source_info"_cs, s->sourceInfoJsonObj());
    auto ctr = new Util::JsonObject();
    ctr->emplace("type"_cs, "extern");
    ctr->emplace("value"_cs, em->object->controlPlaneName());
    parameters->append(ctr);
    auto index = ctxt->conv->convert(mc->arguments->at(0)->expression);
    parameters->append(index);
    return primitive;
}

Util::IJson *ExternConverter_DirectCounter::convertExternObject(
    UNUSED ConversionContext *ctxt, UNUSED const P4::ExternMethod *em,
    UNUSED const IR::MethodCallExpression *mc, UNUSED const IR::StatOrDecl *s,
    UNUSED const bool &emitExterns) {
    if (mc->arguments->size() != 0) {
        modelError("Expected 0 argument for %1%", mc);
        return nullptr;
    }
    // Do not generate any code for this operation
    return nullptr;
}

Util::IJson *ExternConverter_Meter::convertExternObject(UNUSED ConversionContext *ctxt,
                                                        UNUSED const P4::ExternMethod *em,
                                                        UNUSED const IR::MethodCallExpression *mc,
                                                        UNUSED const IR::StatOrDecl *s,
                                                        UNUSED const bool &emitExterns) {
    if (mc->arguments->size() != 1 && mc->arguments->size() != 2) {
        modelError("Expected 2 arguments for %1%", mc);
        return nullptr;
    }
    auto primitive = mkPrimitive("_" + em->originalExternType->name + "_" + em->method->name);
    auto parameters = mkParameters(primitive);
    primitive->emplace_non_null("source_info"_cs, s->sourceInfoJsonObj());
    auto mtr = new Util::JsonObject();
    mtr->emplace("type"_cs, "extern");
    mtr->emplace("value"_cs, em->object->controlPlaneName());
    parameters->append(mtr);
    if (mc->arguments->size() == 2) {
        auto result = ctxt->conv->convert(mc->arguments->at(1)->expression);
        parameters->append(result);
    }
    auto index = ctxt->conv->convert(mc->arguments->at(0)->expression);
    parameters->append(index);
    return primitive;
}

Util::IJson *ExternConverter_DirectMeter::convertExternObject(
    UNUSED ConversionContext *ctxt, UNUSED const P4::ExternMethod *em,
    UNUSED const IR::MethodCallExpression *mc, UNUSED const IR::StatOrDecl *s,
    UNUSED const bool &emitExterns) {
    if (mc->arguments->size() != 1) {
        modelError("Expected 1 argument for %1%", mc);
        return nullptr;
    }
    auto dest = mc->arguments->at(0);
    ctxt->structure->directMeterMap.setDestination(em->object, dest->expression);
    // Do not generate any code for this operation
    return nullptr;
}

Util::IJson *ExternConverter_Register::convertExternObject(
    UNUSED ConversionContext *ctxt, UNUSED const P4::ExternMethod *em,
    UNUSED const IR::MethodCallExpression *mc, UNUSED const IR::StatOrDecl *s,
    UNUSED const bool &emitExterns) {
    if (em->method->name != "write" && em->method->name != "read") {
        modelError("Unsupported register method %1%", mc);
        return nullptr;
    } else if (em->method->name == "write" && mc->arguments->size() != 2) {
        modelError("Expected 2 arguments for %1%", mc);
        return nullptr;
    } else if (em->method->name == "read" && mc->arguments->size() != 2) {
        modelError("p4c-psa internally requires 2 arguments for %1%", mc);
        return nullptr;
    }
    auto reg = new Util::JsonObject();
    reg->emplace("type"_cs, "register_array");
    cstring name = em->object->controlPlaneName();
    reg->emplace("value"_cs, name);
    if (em->method->name == "read") {
        auto primitive = mkPrimitive("register_read"_cs);
        auto parameters = mkParameters(primitive);
        primitive->emplace_non_null("source_info"_cs, s->sourceInfoJsonObj());
        auto dest = ctxt->conv->convert(mc->arguments->at(0)->expression);
        parameters->append(dest);
        parameters->append(reg);
        auto index = ctxt->conv->convert(mc->arguments->at(1)->expression);
        parameters->append(index);
        return primitive;
    } else if (em->method->name == "write") {
        auto primitive = mkPrimitive("register_write"_cs);
        auto parameters = mkParameters(primitive);
        primitive->emplace_non_null("source_info"_cs, s->sourceInfoJsonObj());
        parameters->append(reg);
        auto index = ctxt->conv->convert(mc->arguments->at(0)->expression);
        parameters->append(index);
        auto value = ctxt->conv->convert(mc->arguments->at(1)->expression);
        parameters->append(value);
        return primitive;
    }
    return nullptr;
}

Util::IJson *ExternConverter_Random::convertExternObject(UNUSED ConversionContext *ctxt,
                                                         UNUSED const P4::ExternMethod *em,
                                                         UNUSED const IR::MethodCallExpression *mc,
                                                         UNUSED const IR::StatOrDecl *s,
                                                         UNUSED const bool &emitExterns) {
    if (mc->arguments->size() != 3) {
        modelError("Expected 3 arguments for %1%", mc);
        return nullptr;
    }
    auto primitive = mkPrimitive("modify_field_rng_uniform"_cs);
    auto params = mkParameters(primitive);
    primitive->emplace_non_null("source_info"_cs, em->method->sourceInfoJsonObj());
    auto dest = ctxt->conv->convert(mc->arguments->at(0)->expression);
    /* TODO */
    // auto lo = ctxt->conv->convert(mc->arguments->at(1)->expression);
    // auto hi = ctxt->conv->convert(mc->arguments->at(2)->expression);
    params->append(dest);
    // params->append(lo);
    // params->append(hi);
    return primitive;
}

Util::IJson *ExternConverter_Digest::convertExternObject(UNUSED ConversionContext *ctxt,
                                                         UNUSED const P4::ExternMethod *em,
                                                         UNUSED const IR::MethodCallExpression *mc,
                                                         UNUSED const IR::StatOrDecl *s,
                                                         UNUSED const bool &emitExterns) {
    if (mc->arguments->size() != 1) {
        modelError("Expected 1 arguments for %1%", mc);
        return nullptr;
    }
    auto primitive = mkPrimitive("generate_digest"_cs);
    auto parameters = mkParameters(primitive);
    primitive->emplace_non_null("source_info"_cs, em->method->sourceInfoJsonObj());
    cstring listName = "digest"_cs;
    // If we are supplied a type argument that is a named type use
    // that for the list name.
    if (mc->typeArguments->size() == 1) {
        auto typeArg = mc->typeArguments->at(0);
        if (typeArg->is<IR::Type_Name>()) {
            auto origType = ctxt->refMap->getDeclaration(typeArg->to<IR::Type_Name>()->path, true);
            if (!origType->is<IR::Type_Struct>()) {
                modelError("%1%: expected a struct type", origType->getNode());
                return nullptr;
            }
            auto st = origType->to<IR::Type_Struct>();
            listName = st->controlPlaneName();
        }
    }
    int id = ctxt->createFieldList(mc->arguments->at(0)->expression, listName, true);
    auto cst = new IR::Constant(id);
    ctxt->typeMap->setType(cst, IR::Type_Bits::get(32));
    auto jcst = ctxt->conv->convert(cst);
    parameters->append(jcst);
    return primitive;
}

void ExternConverter_Hash::convertExternInstance(ConversionContext *ctxt, const IR::Declaration *c,
                                                 const IR::ExternBlock *eb,
                                                 UNUSED const bool &emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    auto psaStructure = static_cast<PsaCodeGenerator *>(ctxt->structure);

    // add hash instance
    auto jhash = new Util::JsonObject();
    jhash->emplace("name"_cs, name);
    jhash->emplace("id"_cs, nextId("extern_instances"_cs));
    jhash->emplace("type"_cs, eb->getName());
    jhash->emplace_non_null("source_info"_cs, inst->sourceInfoJsonObj());
    ctxt->json->externs->append(jhash);

    // add attributes
    if (eb->getConstructorParameters()->size() != 1) {
        modelError("%1%: expected one parameter", eb);
        return;
    }

    Util::JsonArray *arr = ctxt->json->insert_array_field(jhash, "attribute_values"_cs);

    auto algo = eb->findParameterValue("algo"_cs);
    CHECK_NULL(algo);
    if (!algo->is<IR::Declaration_ID>()) {
        modelError("%1%: expected a declaration", algo->getNode());
        return;
    }
    cstring algo_name = algo->to<IR::Declaration_ID>()->name;
    algo_name = psaStructure->convertHashAlgorithm(algo_name);
    auto k = new Util::JsonObject();
    k->emplace("name"_cs, "algo");
    k->emplace("type"_cs, "string");
    k->emplace("value"_cs, algo_name);
    arr->append(k);
}

void ExternConverter_Checksum::convertExternInstance(UNUSED ConversionContext *ctxt,
                                                     UNUSED const IR::Declaration *c,
                                                     UNUSED const IR::ExternBlock *eb,
                                                     UNUSED const bool &emitExterns) { /* TODO */ }

void ExternConverter_InternetChecksum::convertExternInstance(UNUSED ConversionContext *ctxt,
                                                             UNUSED const IR::Declaration *c,
                                                             UNUSED const IR::ExternBlock *eb,
                                                             UNUSED const bool &emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    auto trim = inst->controlPlaneName().find(".");
    auto block = inst->controlPlaneName().trim(trim);
    auto psaStructure = static_cast<PsaProgramStructure *>(ctxt->structure);
    auto ingressParser = psaStructure->parsers.at("ingress"_cs)->controlPlaneName();
    auto ingressDeparser = psaStructure->deparsers.at("ingress"_cs)->controlPlaneName();
    auto egressParser = psaStructure->parsers.at("egress"_cs)->controlPlaneName();
    auto egressDeparser = psaStructure->deparsers.at("egress"_cs)->controlPlaneName();
    if (block != ingressParser && block != ingressDeparser && block != egressParser &&
        block != egressDeparser) {
        ::error(ErrorType::ERR_UNSUPPORTED, "%1%: not supported in pipeline on this target", eb);
    }
    // add checksum instance
    auto jcksum = new Util::JsonObject();
    jcksum->emplace("name"_cs, name);
    jcksum->emplace("id"_cs, nextId("extern_instances"_cs));
    jcksum->emplace("type"_cs, eb->getName());
    jcksum->emplace_non_null("source_info"_cs, inst->sourceInfoJsonObj());
    ctxt->json->externs->append(jcksum);
}

void ExternConverter_Counter::convertExternInstance(UNUSED ConversionContext *ctxt,
                                                    UNUSED const IR::Declaration *c,
                                                    UNUSED const IR::ExternBlock *eb,
                                                    UNUSED const bool &emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    auto sz = eb->findParameterValue("n_counters"_cs);
    CHECK_NULL(sz);
    if (!sz->is<IR::Constant>()) {
        modelError("%1%: expected a constant", sz->getNode());
        return;
    }

    // adding counter instance to counter_arrays[]
    auto jctr = new Util::JsonObject();
    jctr->emplace("name"_cs, name);
    jctr->emplace("id"_cs, nextId("counter_arrays"_cs));
    jctr->emplace_non_null("source_info"_cs, eb->sourceInfoJsonObj());
    jctr->emplace("size"_cs, sz->to<IR::Constant>()->value);
    jctr->emplace("is_direct"_cs, false);
    ctxt->json->counters->append(jctr);

    // add counter instance to extern_instances
    auto extern_obj = new Util::JsonObject();
    extern_obj->emplace("name"_cs, name);
    extern_obj->emplace("id"_cs, nextId("extern_instances"_cs));
    extern_obj->emplace("type"_cs, eb->getName());
    extern_obj->emplace("source_info"_cs, inst->sourceInfoJsonObj());
    ctxt->json->externs->append(extern_obj);
    Util::JsonArray *arr = ctxt->json->insert_array_field(extern_obj, "attribute_values"_cs);

    if (eb->getConstructorParameters()->size() != 2) {
        modelError("%1%: expected two parameters", eb);
        return;
    }
    // first argument to create a counter is just a number, convert and dump to json
    // we get a name from param, type and value from the arguments
    auto attr_obj = new Util::JsonObject();
    auto arg1 = sz->to<IR::Constant>();
    auto param1 = eb->getConstructorParameters()->getParameter(0);
    auto bitwidth = ctxt->typeMap->widthBits(arg1->type, sz->getNode(), false);
    cstring repr = BMV2::stringRepr(arg1->value, ROUNDUP(bitwidth, 8));
    attr_obj->emplace("name"_cs, param1->toString());
    attr_obj->emplace("type"_cs, "hexstr");
    attr_obj->emplace("value"_cs, repr);
    arr->append(attr_obj);

    // second argument is the counter type, this is psa metadata, the converter
    // in conversion context will handle that for us
    auto tp = eb->findParameterValue("type"_cs);
    if (!tp || !tp->is<IR::Declaration_ID>()) {
        modelError("%1%: expected a declaration_id", tp ? tp->getNode() : eb->getNode());
        return;
    }
    auto arg2 = tp->to<IR::Declaration_ID>();
    auto param2 = eb->getConstructorParameters()->getParameter(1);
    auto mem = arg2->toString();
    LOG5("In convertParam with p2 " << param2->toString() << " and mem " << mem);
    auto jsn = ctxt->conv->convertParam(param2, mem);
    arr->append(jsn);
}

void ExternConverter_DirectCounter::convertExternInstance(UNUSED ConversionContext *ctxt,
                                                          UNUSED const IR::Declaration *c,
                                                          UNUSED const IR::ExternBlock *eb,
                                                          UNUSED const bool &emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    auto it = ctxt->structure->directCounterMap.find(name);
    if (it == ctxt->structure->directCounterMap.end()) {
        ::warning(ErrorType::WARN_UNUSED, "%1%: Direct counter not used; ignoring", inst);
    } else {
        auto jctr = new Util::JsonObject();
        jctr->emplace("name"_cs, name);
        jctr->emplace("id"_cs, nextId("counter_arrays"_cs));
        jctr->emplace_non_null("source_info"_cs, eb->sourceInfoJsonObj());
        jctr->emplace("is_direct"_cs, true);
        jctr->emplace("binding"_cs, it->second->controlPlaneName());
        ctxt->json->counters->append(jctr);

        // Adding direct counter to EXTERN_INSTANCES

        auto extern_obj = new Util::JsonObject();
        extern_obj->emplace("name"_cs, name);
        extern_obj->emplace("id"_cs, nextId("extern_instances"_cs));
        extern_obj->emplace("type"_cs, eb->getName());
        extern_obj->emplace("source_info"_cs, inst->sourceInfoJsonObj());
        ctxt->json->externs->append(extern_obj);
        Util::JsonArray *arr = ctxt->json->insert_array_field(extern_obj, "attribute_values"_cs);

        // Direct Counter only has a single argument, which is psa metadata
        // converter in conversion context will handle this for us
        auto tp = eb->findParameterValue("type"_cs);
        if (!tp || !tp->is<IR::Declaration_ID>()) {
            modelError("%1%: expected a declaration_id", tp ? tp->getNode() : eb->getNode());
            return;
        }
        if (eb->getConstructorParameters()->size() < 2) {
            modelError("%1%: expected 2 parameters", eb);
            return;
        }
        auto arg = tp->to<IR::Declaration_ID>();
        auto param = eb->getConstructorParameters()->getParameter(1);
        auto mem = arg->toString();
        LOG5("In convertParam with param " << param->toString() << " and mem " << mem);
        auto jsn = ctxt->conv->convertParam(param, mem);
        arr->append(jsn);
    }
}

void ExternConverter_Meter::convertExternInstance(UNUSED ConversionContext *ctxt,
                                                  UNUSED const IR::Declaration *c,
                                                  UNUSED const IR::ExternBlock *eb,
                                                  UNUSED const bool &emitExterns) {
    if (eb->getConstructorParameters()->size() != 2) {
        modelError("%1%: expected two parameters", eb);
        return;
    }

    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();

    // adding meter instance into extern_instances
    auto jext_mtr = new Util::JsonObject();
    jext_mtr->emplace("name"_cs, name);
    jext_mtr->emplace("id"_cs, nextId("extern_instances"_cs));
    jext_mtr->emplace("type"_cs, eb->getName());
    jext_mtr->emplace_non_null("source_info"_cs, eb->sourceInfoJsonObj());
    ctxt->json->externs->append(jext_mtr);

    // adding attributes to meter extern_instance
    Util::JsonArray *arr = ctxt->json->insert_array_field(jext_mtr, "attribute_values"_cs);

    // is_direct
    auto is_direct = new Util::JsonObject();
    is_direct->emplace("name"_cs, "is_direct");
    is_direct->emplace("type"_cs, "hexstr");
    is_direct->emplace("value"_cs, 0);
    arr->append(is_direct);

    // meter_array size
    auto sz = eb->findParameterValue("n_meters"_cs);
    CHECK_NULL(sz);
    if (!sz->is<IR::Constant>()) {
        modelError("%1%: expected a constant", sz->getNode());
        return;
    }
    auto attr_name = eb->getConstructorParameters()->getParameter(0);
    auto s = sz->to<IR::Constant>();
    auto bitwidth = ctxt->typeMap->widthBits(s->type, sz->getNode(), false);
    cstring val = BMV2::stringRepr(s->value, ROUNDUP(bitwidth, 8));
    auto msz = new Util::JsonObject();
    msz->emplace("name"_cs, attr_name->toString());
    msz->emplace("type"_cs, "hexstr");
    msz->emplace("value"_cs, val);
    arr->append(msz);

    // rate count
    auto rc = new Util::JsonObject();
    rc->emplace("name"_cs, "rate_count");
    rc->emplace("type"_cs, "hexstr");
    rc->emplace("value"_cs, 2);
    arr->append(rc);

    // meter kind
    auto mkind = eb->findParameterValue("type"_cs);
    CHECK_NULL(mkind);
    if (!mkind->is<IR::Declaration_ID>()) {
        modelError("%1%: expected a member", mkind->getNode());
        return;
    }
    cstring mkind_name = mkind->to<IR::Declaration_ID>()->name;
    cstring type;
    if (mkind_name == "PACKETS")
        type = "packets"_cs;
    else if (mkind_name == "BYTES")
        type = "bytes"_cs;
    else
        ::error(ErrorType::ERR_UNEXPECTED, "%1%: unexpected meter type", mkind->getNode());
    auto k = new Util::JsonObject();
    k->emplace("name"_cs, "type");
    k->emplace("type"_cs, "string");
    k->emplace("value"_cs, type);
    arr->append(k);
}

void ExternConverter_DirectMeter::convertExternInstance(UNUSED ConversionContext *ctxt,
                                                        UNUSED const IR::Declaration *c,
                                                        UNUSED const IR::ExternBlock *eb,
                                                        UNUSED const bool &emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    auto info = ctxt->structure->directMeterMap.getInfo(c);
    CHECK_NULL(info);
    CHECK_NULL(info->table);
    CHECK_NULL(info->destinationField);

    auto jmtr = new Util::JsonObject();
    jmtr->emplace("name"_cs, name);
    jmtr->emplace("id"_cs, nextId("meter_arrays"_cs));
    jmtr->emplace_non_null("source_info"_cs, eb->sourceInfoJsonObj());
    jmtr->emplace("is_direct"_cs, true);
    jmtr->emplace("rate_count"_cs, 2);
    auto mkind = eb->findParameterValue("type"_cs);
    CHECK_NULL(mkind);
    if (!mkind->is<IR::Declaration_ID>()) {
        modelError("%1%: expected a member", mkind->getNode());
        return;
    }
    cstring mkind_name = mkind->to<IR::Declaration_ID>()->name;
    cstring type;
    if (mkind_name == "PACKETS") {
        type = "packets"_cs;
    } else if (mkind_name == "BYTES") {
        type = "bytes"_cs;
    } else {
        modelError("%1%: unexpected meter type", mkind->getNode());
        return;
    }
    jmtr->emplace("type"_cs, type);
    jmtr->emplace("size"_cs, info->tableSize);
    cstring tblname = info->table->controlPlaneName();
    jmtr->emplace("binding"_cs, tblname);
    auto result = ctxt->conv->convert(info->destinationField);
    jmtr->emplace("result_target"_cs, result->to<Util::JsonObject>()->get("value"_cs));
    ctxt->json->meter_arrays->append(jmtr);
}

void ExternConverter_Register::convertExternInstance(UNUSED ConversionContext *ctxt,
                                                     UNUSED const IR::Declaration *c,
                                                     UNUSED const IR::ExternBlock *eb,
                                                     UNUSED const bool &emitExterns) {
    size_t paramSize = eb->getConstructorParameters()->size();
    if (paramSize == 2) {
        modelError("%1%: Expecting 1 parameter. Initial value not supported", eb->constructor);
    }
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    auto jreg = new Util::JsonObject();
    jreg->emplace("name"_cs, name);
    jreg->emplace("id"_cs, nextId("register_arrays"_cs));
    jreg->emplace_non_null("source_info"_cs, eb->sourceInfoJsonObj());
    auto sz = eb->findParameterValue("size"_cs);
    CHECK_NULL(sz);
    if (!sz->is<IR::Constant>()) {
        modelError("%1%: expected a constant", sz->getNode());
        return;
    }
    if (sz->to<IR::Constant>()->value == 0)
        error(ErrorType::ERR_UNSUPPORTED, "%1%: direct registers are not supported", inst);
    jreg->emplace("size"_cs, sz->to<IR::Constant>()->value);
    if (!eb->instanceType->is<IR::Type_SpecializedCanonical>()) {
        modelError("%1%: Expected a generic specialized type", eb->instanceType);
        return;
    }
    auto st = eb->instanceType->to<IR::Type_SpecializedCanonical>();
    if (st->arguments->size() != 2) {
        modelError("%1%: expected 2 type argument", st);
        return;
    }
    auto regType = st->arguments->at(0);
    if (!regType->is<IR::Type_Bits>()) {
        ::error(ErrorType::ERR_UNSUPPORTED,
                "%1%: registers with types other than bit<> or int<> are not suppoted", eb);
        return;
    }
    unsigned width = regType->width_bits();
    if (width == 0) {
        ::error(ErrorType::ERR_UNKNOWN, "%1%: unknown width", st->arguments->at(0));
        return;
    }
    jreg->emplace("bitwidth"_cs, width);
    ctxt->json->register_arrays->append(jreg);
}

void ExternConverter_Random::convertExternInstance(UNUSED ConversionContext *ctxt,
                                                   UNUSED const IR::Declaration *c,
                                                   UNUSED const IR::ExternBlock *eb,
                                                   UNUSED const bool &emitExterns) { /* TODO */ }

void ExternConverter_ActionProfile::convertExternInstance(UNUSED ConversionContext *ctxt,
                                                          UNUSED const IR::Declaration *c,
                                                          UNUSED const IR::ExternBlock *eb,
                                                          UNUSED const bool &emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    // Might call this multiple times if the selector/profile is used more than
    // once in a pipeline, so only add it to the action_profiles once
    if (BMV2::JsonObjects::find_object_by_name(ctxt->action_profiles, name)) return;
    auto action_profile = new Util::JsonObject();
    action_profile->emplace("name"_cs, name);
    action_profile->emplace("id"_cs, nextId("action_profiles"_cs));
    action_profile->emplace_non_null("source_info"_cs, eb->sourceInfoJsonObj());

    auto sz = eb->findParameterValue("size"_cs);
    BUG_CHECK(sz, "%1%Invalid declaration of extern ActionProfile ctor: no size param",
              eb->constructor->srcInfo);

    if (!sz->is<IR::Constant>()) {
        ::error(ErrorType::ERR_EXPECTED, "%1%: expected a constant", sz);
    }
    action_profile->emplace("max_size"_cs, sz->to<IR::Constant>()->value);

    ctxt->action_profiles->append(action_profile);
}

void ExternConverter_ActionSelector::convertExternInstance(UNUSED ConversionContext *ctxt,
                                                           UNUSED const IR::Declaration *c,
                                                           UNUSED const IR::ExternBlock *eb,
                                                           UNUSED const bool &emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    // Might call this multiple times if the selector/profile is used more than
    // once in a pipeline, so only add it to the action_profiles once
    if (BMV2::JsonObjects::find_object_by_name(ctxt->action_profiles, name)) return;
    auto action_profile = new Util::JsonObject();
    action_profile->emplace("name"_cs, name);
    action_profile->emplace("id"_cs, nextId("action_profiles"_cs));
    action_profile->emplace_non_null("source_info"_cs, eb->sourceInfoJsonObj());

    auto sz = eb->findParameterValue("size"_cs);
    BUG_CHECK(sz, "%1%Invalid declaration of extern ActionSelector: no size param",
              eb->constructor->srcInfo);
    if (!sz->is<IR::Constant>()) {
        ::error(ErrorType::ERR_EXPECTED, "%1%: expected a constant", sz);
        return;
    }
    action_profile->emplace("max_size"_cs, sz->to<IR::Constant>()->value);

    auto selector = new Util::JsonObject();
    auto hash = eb->findParameterValue("algo"_cs);

    if (!hash->is<IR::Declaration_ID>()) {
        modelError("%1%: expected a member", hash->getNode());
        return;
    }
    auto algo = ExternConverter::convertHashAlgorithm(hash->to<IR::Declaration_ID>()->name);
    selector->emplace("algo"_cs, algo);
    auto input = ctxt->get_selector_input(c->to<IR::Declaration_Instance>());
    if (input == nullptr) {
        // the selector is never used by any table, we cannot figure out its
        // input and therefore cannot include it in the JSON
        ::warning(ErrorType::WARN_UNUSED,
                  "Action selector '%1%' is never referenced by a table "
                  "and cannot be included in bmv2 JSON",
                  c);
        return;
    }
    auto j_input = mkArrayField(selector, "input"_cs);
    for (auto expr : *input) {
        auto jk = ctxt->conv->convert(expr);
        j_input->append(jk);
    }
    action_profile->emplace("selector"_cs, selector);

    ctxt->action_profiles->append(action_profile);
}

void ExternConverter_Digest::convertExternInstance(UNUSED ConversionContext *ctxt,
                                                   UNUSED const IR::Declaration *c,
                                                   UNUSED const IR::ExternBlock *eb,
                                                   UNUSED const bool &emitExterns) { /* TODO */ }

}  // namespace BMV2
