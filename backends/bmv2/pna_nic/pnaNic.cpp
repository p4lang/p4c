/*
Copyright 2024 Marvell Technology, Inc.

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

#include "pnaNic.h"

#include "frontends/common/model.h"
#include "frontends/p4/cloner.h"

namespace BMV2 {

using namespace P4::literals;

void PnaCodeGenerator::create(ConversionContext *ctxt, PortableProgramStructure *structure) {
    createTypes(ctxt, structure);
    createHeaders(ctxt, structure);
    createScalars(ctxt, structure);
    createExterns();
    createParsers(ctxt, structure);
    createActions(ctxt, structure);
    createControls(ctxt, structure);
    createDeparsers(ctxt, structure);
    createGlobals();
}

void PnaCodeGenerator::createParsers(ConversionContext *ctxt, PortableProgramStructure *structure) {
    {
        auto cvt = new ParserConverter(ctxt, "main_parser"_cs);
        auto main_control = structure->parsers.at("main_parser"_cs);
        main_control->apply(*cvt);
    }
}

void PnaCodeGenerator::createControls(ConversionContext *ctxt, PortableProgramStructure *structure) {
    auto cvt = new BMV2::ControlConverter<Standard::Arch::PNA>(ctxt, "main_control"_cs, true);
    auto main_control = structure->pipelines.at("main_control"_cs);
    main_control->apply(*cvt);
}

void PnaCodeGenerator::createDeparsers(ConversionContext *ctxt, PortableProgramStructure *structure) {
    {
        auto cvt = new DeparserConverter(ctxt, "main_deparser"_cs);
        auto main_control = structure->deparsers.at("main_deparser"_cs);
        main_control->apply(*cvt);
    }
}

void PnaNicBackend::convert(const IR::ToplevelBlock *tlb) {
    CHECK_NULL(tlb);
    PnaProgramStructure structure(refMap, typeMap);

    auto parsePnaArch = new ParsePnaArchitecture(&structure);
    auto main = tlb->getMain();
    if (!main) return;

    if (main->type->name != "PNA_NIC")
        ::warning(ErrorType::WARN_INVALID,
                  "%1%: the main package should be called PNA_NIC"
                  "; are you using the wrong architecture?",
                  main->type->name);

    main->apply(*parsePnaArch);

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
        new P4::SimplifyControlFlow(typeMap),
        new LowerExpressions(typeMap),
        new PassRepeated(
            {new P4::ConstantFolding(refMap, typeMap), new P4::StrengthReduction(typeMap)}),
        new P4::TypeChecking(refMap, typeMap),
        new P4::RemoveComplexExpressions(refMap, typeMap,
                                         new ProcessControls(&structure.pipeline_controls)),
        new P4::SimplifyControlFlow(typeMap),
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
    toplevel->apply(*new P4::BuildResourceMap(&structure.resourceMap));

    main = toplevel->getMain();
    if (!main) return;  // no main
    main->apply(*parsePnaArch);
    if (::errorCount() > 0) return;
    program = toplevel->getProgram();

    PassManager toJson = {new P4::DiscoverStructure(&structure),
                          new InspectPnaProgram(refMap, typeMap, &structure),
                          new ConvertPnaToJson(refMap, typeMap, toplevel, json, &structure)};
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

ExternConverter_Hash ExternConverter_Hash::singleton;
ExternConverter_InternetChecksum ExternConverter_InternetChecksum::singleton;
ExternConverter_Register ExternConverter_Register::singleton;

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
        modelError("p4c-pna internally requires 2 arguments for %1%", mc);
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

void ExternConverter_Hash::convertExternInstance(ConversionContext *ctxt, const IR::Declaration *c,
                                                 const IR::ExternBlock *eb,
                                                 UNUSED const bool &emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    // auto pnaStructure = static_cast<PnaCodeGenerator *>(ctxt->structure);
    auto pnaStructure = new PnaCodeGenerator();

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
    algo_name = pnaStructure->convertHashAlgorithm(algo_name);
    auto k = new Util::JsonObject();
    k->emplace("name"_cs, "algo");
    k->emplace("type"_cs, "string");
    k->emplace("value"_cs, algo_name);
    arr->append(k);
}

void ExternConverter_InternetChecksum::convertExternInstance(UNUSED ConversionContext *ctxt,
                                                             UNUSED const IR::Declaration *c,
                                                             UNUSED const IR::ExternBlock *eb,
                                                             UNUSED const bool &emitExterns) {
    auto inst = c->to<IR::Declaration_Instance>();
    cstring name = inst->controlPlaneName();
    auto trim = inst->controlPlaneName().find(".");
    auto block = inst->controlPlaneName().trim(trim);
    auto pnaStructure = static_cast<PnaProgramStructure *>(ctxt->structure);
    auto mainParser = pnaStructure->parsers.at("main_parser"_cs)->controlPlaneName();
    auto mainDeparser = pnaStructure->deparsers.at("main_deparser"_cs)->controlPlaneName();
    if (block != mainParser && block != mainDeparser) {
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

}  // namespace BMV2
