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

#include "frontends/common/model.h"
#include "portableSwitch.h"

namespace BMV2 {

void PsaProgramStructure::create(ConversionContext* ctxt, const IR::P4Program* program, cstring scalarsName) {
    createTypes(ctxt);
    createHeaders(ctxt);
    //auto hconv = new HeaderConverter(ctxt, scalarsName);
    //program->apply(*hconv);
    createExterns();
    createParsers(ctxt);
    createActions();
    createControls(ctxt);
    createDeparsers(ctxt);
    createGlobals();
}

void PsaProgramStructure::createStructLike(ConversionContext* ctxt, const IR::Type_StructLike* st) {
    CHECK_NULL(st);
    cstring name = st->controlPlaneName();
    unsigned max_length = 0;  // for variable-sized headers
    bool varbitFound = false;
    auto fields = new Util::JsonArray();
    for (auto f : st->fields) {
        auto field = new Util::JsonArray();
        auto ftype = typeMap->getType(f, true);
        if (ftype->to<IR::Type_StructLike>()) {
            BUG("%1%: nested structure", st);
        } else if (ftype->is<IR::Type_Boolean>()) {
            field->append(f->name.name);
            field->append(1);
            field->append(0);
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
                ::error("%1%: headers with multiple varbit fields not supported", st);
            varbitFound = true;
        } else if (auto type = ftype->to<IR::Type_Error>()) {
            field->append(f->name.name);
            field->append(error_width);
            field->append(0);
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

void PsaProgramStructure::createTypes(ConversionContext* ctxt) {
    for (auto kv : header_types)
        createStructLike(ctxt, kv.second);
    for (auto kv : metadata_types)
        createStructLike(ctxt, kv.second);
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
    // add errors to json
    // add enums to json
}

void PsaProgramStructure::createHeaders(ConversionContext* ctxt) {
    for (auto kv : headers) {
        auto type = kv.second->type->to<IR::Type_StructLike>();
        ctxt->json->add_header(type->controlPlaneName(), kv.second->name);
    }
    for (auto kv : metadata) {
        auto type = kv.second->type->to<IR::Type_StructLike>();
        ctxt->json->add_header(type->controlPlaneName(), kv.second->name);
    }
    for (auto kv : header_stacks) {
        // json->add_header_stack(stack_type, stack_name, stack_size, ids);
    }
    for (auto kv : header_unions) {
        auto header_name = kv.first;
        auto header_type = kv.second->to<IR::Type_StructLike>()->controlPlaneName();
        // We have to add separately a header instance for all
        // headers in the union.  Each instance will be named with
        // a prefix including the union name, e.g., "u.h"
        Util::JsonArray* fields = new Util::JsonArray();
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

void PsaProgramStructure::createParsers(ConversionContext* ctxt) {
    auto cvt = new ParserConverter(ctxt);
    for (auto kv : parsers) {
        kv.second->apply(*cvt);
    }
}

void PsaProgramStructure::createExterns() {
    // add parse_vsets to json
    // add meter_arrays to json
    // add counter_arrays to json
    // add register_arrays to json
    // add checksums to json
    // add learn_list to json
    // add calculations to json
    // add extern_instances to json
}

void PsaProgramStructure::createActions() {
    // add actions to json
}

void PsaProgramStructure::createControls(ConversionContext* ctxt) {
    auto cvt = new BMV2::ControlConverter(ctxt, "ingress", true);
    auto ingress = pipelines.at("ingress");
    ingress->apply(*cvt);

    cvt = new BMV2::ControlConverter(ctxt, "egress", true);
    auto egress = pipelines.at("egress");
    egress->apply(*cvt);
}

void PsaProgramStructure::createDeparsers(ConversionContext* ctxt) {
    auto cvt = new DeparserConverter(ctxt);
    auto ingress = deparsers.at("ingress");
    ingress->apply(*cvt);
    auto egress = deparsers.at("egress");
    egress->apply(*cvt);
}

void PsaProgramStructure::createGlobals() {
    for (auto e : globals) {
        // convertExternInstances(e->node->to<IR::Declaration>(), e->to<IR::ExternBlock>());
    }
}

bool ParsePsaArchitecture::preorder(const IR::ToplevelBlock* block) {
    /// Blocks are not in IR tree, use a custom visitor to traverse
    for (auto it : block->constantValue) {
        if (it.second->is<IR::Block>())
            visit(it.second->getNode());
    }
    return false;
}

bool ParsePsaArchitecture::preorder(const IR::ExternBlock* block) {
    if (block->node->is<IR::Declaration>())
        structure->globals.push_back(block);
    return false;
}

bool ParsePsaArchitecture::preorder(const IR::PackageBlock* block) {
    auto pkg = block->getParameterValue("ingress");
    if (auto ingress = pkg->to<IR::PackageBlock>()) {
        auto parser = ingress->getParameterValue("ip")->to<IR::ParserBlock>();
        auto pipeline = ingress->getParameterValue("ig")->to<IR::ControlBlock>();
        auto deparser = ingress->getParameterValue("id")->to<IR::ControlBlock>();
        structure->block_type.emplace(parser->container, std::make_pair(INGRESS, PARSER));
        structure->block_type.emplace(pipeline->container, std::make_pair(INGRESS, PIPELINE));
        structure->block_type.emplace(deparser->container, std::make_pair(INGRESS, DEPARSER));
    }
    pkg = block->getParameterValue("egress");
    if (auto egress = pkg->to<IR::PackageBlock>()) {
        auto parser = egress->getParameterValue("ep")->to<IR::ParserBlock>();
        auto pipeline = egress->getParameterValue("eg")->to<IR::ControlBlock>();
        auto deparser = egress->getParameterValue("ed")->to<IR::ControlBlock>();
        structure->block_type.emplace(parser->container, std::make_pair(EGRESS, PARSER));
        structure->block_type.emplace(pipeline->container, std::make_pair(EGRESS, PIPELINE));
        structure->block_type.emplace(deparser->container, std::make_pair(EGRESS, DEPARSER));
    }
    return false;
}

void InspectPsaProgram::postorder(const IR::Type_Header* h) {
    // inspect IR::Type_Header
    // populate structure->header_types;
}

void InspectPsaProgram::postorder(const IR::Type_HeaderUnion* hu) {
    // inspect IR::Type_HeaderUnion
    // populate structure->header_union_types;
}

void InspectPsaProgram::postorder(const IR::Declaration_Variable* var) {
    // inspect IR::Declaration_Variable
    // populate structure->headers or
    //          structure->header_stacks or
    //          structure->header_unions
    // based on the type of the variable
}

void InspectPsaProgram::postorder(const IR::Declaration_Instance* di) {
    // inspect IR::Declaration_Instance,
    // populate structure->meter_arrays or
    //          structure->counter_arrays or
    //          structure->register_arrays or
    //          structure->extern_instances or
    //          structure->checksums
    // based on the type of the instance
}

void InspectPsaProgram::postorder(const IR::P4Action* act) {
    // inspect IR::P4Action,
    // populate structure->actions
}

void InspectPsaProgram::postorder(const IR::Type_Error* err) {
    // inspect IR::Type_Error
    // populate structure->errors.
}

bool InspectPsaProgram::isHeaders(const IR::Type_StructLike* st) {
    bool result = false;
    for (auto f : st->fields) {
        if (f->type->is<IR::Type_Header>() || f->type->is<IR::Type_Stack>()) {
            result = true;
        }
    }
    return result;
}

void InspectPsaProgram::addHeaderType(const IR::Type_StructLike *st) {
    if (st->is<IR::Type_HeaderUnion>()) {
        for (auto f : st->fields) {
            auto ftype = typeMap->getType(f, true);
            auto ht = ftype->to<IR::Type_Header>();
            CHECK_NULL(ht);
            addHeaderType(ht);
        }
        pinfo->header_union_types.emplace(st->getName(), st->to<IR::Type_HeaderUnion>());
        return;
    } else if (st->is<IR::Type_Header>()) {
        pinfo->header_types.emplace(st->getName(), st->to<IR::Type_Header>());
    } else if (st->is<IR::Type_Struct>()) {
        pinfo->metadata_types.emplace(st->getName(), st->to<IR::Type_Struct>());
    }
}

void InspectPsaProgram::addHeaderInstance(const IR::Type_StructLike *st, cstring name) {
    auto inst = new IR::Declaration_Variable(name, st);
    if (st->is<IR::Type_Header>())
        pinfo->headers.emplace(name, inst);
    else if (st->is<IR::Type_Struct>())
        pinfo->metadata.emplace(name, inst);
    else if (st->is<IR::Type_HeaderUnion>())
        pinfo->header_unions.emplace(name, inst);
}

void InspectPsaProgram::addTypesAndInstances(const IR::Type_StructLike* type, bool isHeader) {
    addHeaderType(type);
    addHeaderInstance(type, type->controlPlaneName());
    for (auto f : type->fields) {
        auto ft = typeMap->getType(f, true);
        if (ft->is<IR::Type_StructLike>()) {
            // The headers struct can not contain nested structures.
            if (isHeader && ft->is<IR::Type_Struct>()) {
                ::error("Type %1% should only contain headers, header stacks, or header unions",
                        type);
                return;
            }
            if (auto hft = ft->to<IR::Type_Header>()) {
                addHeaderType(hft);
                addHeaderInstance(hft, hft->controlPlaneName());
            } else if (ft->is<IR::Type_HeaderUnion>()) {
                for (auto uf : ft->to<IR::Type_HeaderUnion>()->fields) {
                    auto uft = typeMap->getType(uf, true);
                    if (auto h_type = uft->to<IR::Type_Header>()) {
                        addHeaderType(h_type);
                        addHeaderInstance(h_type, h_type->controlPlaneName());
                    } else {
                        ::error("Type %1% cannot contain type %2%", ft, uft);
                        return;
                    }
                }
                pinfo->header_union_types.emplace(type->getName(), type->to<IR::Type_HeaderUnion>());
                addHeaderInstance(type, type->controlPlaneName());
            } else {
                LOG1("add struct type " << type);
                pinfo->metadata_types.emplace(type->getName(), type->to<IR::Type_Struct>());
                addHeaderInstance(type, type->controlPlaneName());
            }
        } else if (ft->is<IR::Type_Stack>()) {
            auto stack = ft->to<IR::Type_Stack>();
            auto stack_name = f->controlPlaneName();
            auto stack_size = stack->getSize();
            auto type = typeMap->getTypeType(stack->elementType, true);
            BUG_CHECK(type->is<IR::Type_Header>(), "%1% not a header type", stack->elementType);
            auto ht = type->to<IR::Type_Header>();
            addHeaderType(ht);
            auto stack_type = stack->elementType->to<IR::Type_Header>();
            std::vector<unsigned> ids;
            for (unsigned i = 0; i < stack_size; i++) {
                cstring hdrName = f->controlPlaneName() + "[" + Util::toString(i) + "]";
                // FIXME:
                // auto id = json->add_header(stack_type, hdrName);
                addHeaderInstance(stack_type, hdrName);
                // ids.push_back(id);
            }
            // addHeaderStackInstance();
        } else {
            // Treat this field like a scalar local variable
            cstring newName = refMap->newName(type->getName() + "." + f->name);
            if (ft->is<IR::Type_Bits>()) {
                auto tb = ft->to<IR::Type_Bits>();
                pinfo->scalars_width += tb->size;
                pinfo->scalarMetadataFields.emplace(f, newName);
            } else if (ft->is<IR::Type_Boolean>()) {
                pinfo->scalars_width += 1;
                pinfo->scalarMetadataFields.emplace(f, newName);
            } else if (ft->is<IR::Type_Error>()) {
                pinfo->scalars_width += 32;
                pinfo->scalarMetadataFields.emplace(f, newName);
            } else {
                BUG("%1%: Unhandled type for %2%", ft, f);
            }
        }
    }
}

// This visitor only visits the parameter in the statement from architecture.
void InspectPsaProgram::postorder(const IR::Parameter* param) {
    auto ft = typeMap->getType(param->getNode(), true);
    LOG3("add param " << ft);
    // only convert parameters that are IR::Type_StructLike
    if (!ft->is<IR::Type_StructLike>())
        return;
    auto st = ft->to<IR::Type_StructLike>();
    // parameter must be a type that we have not seen before
    if (pinfo->hasVisited(st))
        return;
    auto isHeader = isHeaders(st);
    addTypesAndInstances(st, isHeader);
}

void InspectPsaProgram::postorder(const IR::P4Parser* p) {
    if (pinfo->block_type.count(p)) {
        auto info = pinfo->block_type.at(p);
        if (info.first == INGRESS && info.second == PARSER)
            pinfo->parsers.emplace("ingress", p);
        else if (info.first == EGRESS && info.second == PARSER)
            pinfo->parsers.emplace("egress", p);
    }
}

void InspectPsaProgram::postorder(const IR::P4Control *c) {
    if (pinfo->block_type.count(c)) {
        auto info = pinfo->block_type.at(c);
        if (info.first == INGRESS && info.second == PIPELINE)
            pinfo->pipelines.emplace("ingress", c);
        else if (info.first == EGRESS && info.second == PIPELINE)
            pinfo->pipelines.emplace("egress", c);
        else if (info.first == INGRESS && info.second == DEPARSER)
            pinfo->deparsers.emplace("ingress", c);
        else if (info.first == EGRESS && info.second == DEPARSER)
            pinfo->deparsers.emplace("egress", c);
    }
}

void PortableSwitchBackend::convert(const IR::ToplevelBlock* tlb) {
    CHECK_NULL(tlb);
    PsaProgramStructure structure(refMap, typeMap);

    auto parsePsaArch = new ParsePsaArchitecture(&structure);
    auto main = tlb->getMain();
    if (!main) return;
    main->apply(*parsePsaArch);

    auto evaluator = new P4::EvaluatorPass(refMap, typeMap);
    auto program = tlb->getProgram();
    PassManager simplify = {
        // new RenameUserMetadata(refMap, userMetaType, userMetaName),
        new P4::ClearTypeMap(typeMap),  // because the user metadata type has changed
        //new P4::SynthesizeActions(refMap, typeMap, new SkipControls(&non_pipeline_controls)),
        new P4::MoveActionsToTables(refMap, typeMap),
        new P4::TypeChecking(refMap, typeMap),
        new P4::SimplifyControlFlow(refMap, typeMap),
        new LowerExpressions(typeMap),
        new P4::ConstantFolding(refMap, typeMap, false),
        new P4::TypeChecking(refMap, typeMap),
        //new RemoveComplexExpressions(refMap, typeMap, new ProcessControls(&pipeline_controls)),
        new P4::SimplifyControlFlow(refMap, typeMap),
        new P4::RemoveAllUnusedDeclarations(refMap),
        evaluator,
        new VisitFunctor([this, evaluator, structure]() { toplevel = evaluator->getToplevelBlock(); }),
    };
    program->apply(simplify);

    // map IR node to compile-time allocated resource blocks.
    toplevel->apply(*new BMV2::BuildResourceMap(&structure.resourceMap));

    PassManager toJson = {
        new DiscoverStructure(&structure),
        new InspectPsaProgram(refMap, typeMap, &structure),
        new ConvertPsaToJson(refMap, typeMap, toplevel, json, &structure)
    };
    program->apply(toJson);

    json->add_program_info(options.file);
    json->add_meta_info();
}

EXTERN_CONVERTER_SINGLETON(Hash)
EXTERN_CONVERTER_SINGLETON(Checksum)
EXTERN_CONVERTER_SINGLETON(InternetChecksum)
EXTERN_CONVERTER_SINGLETON(Counter)
EXTERN_CONVERTER_SINGLETON(DirectCounter)
EXTERN_CONVERTER_SINGLETON(Meter)
EXTERN_CONVERTER_SINGLETON(DirectMeter)
EXTERN_CONVERTER_SINGLETON(Register)
EXTERN_CONVERTER_SINGLETON(Random)
EXTERN_CONVERTER_SINGLETON(ActionProfile)
EXTERN_CONVERTER_SINGLETON(ActionSelector)
EXTERN_CONVERTER_SINGLETON(Digest)

CONVERT_EXTERN_OBJECT(Hash) {
    auto primitive = mkPrimitive("Hash");
    return primitive;
}

CONVERT_EXTERN_OBJECT(Checksum) {
    auto primitive = mkPrimitive("Checksum");
    return primitive;
}
CONVERT_EXTERN_OBJECT(InternetChecksum) {
    auto primitive = mkPrimitive("InternetChecksum");
    return primitive;
}
CONVERT_EXTERN_OBJECT(Counter) {
    auto primitive = mkPrimitive("Counter");
    return primitive;
}
CONVERT_EXTERN_OBJECT(DirectCounter) {
    auto primitive = mkPrimitive("DirectCounter");
    return primitive;
}
CONVERT_EXTERN_OBJECT(Meter) {
    auto primitive = mkPrimitive("Meter");
    return primitive;
}
CONVERT_EXTERN_OBJECT(DirectMeter) {
    auto primitive = mkPrimitive("DirectMeter");
    return primitive;
}
CONVERT_EXTERN_OBJECT(Register) {
    auto primitive = mkPrimitive("Register");
    return primitive;
}
CONVERT_EXTERN_OBJECT(Random) {
    auto primitive = mkPrimitive("Random");
    return primitive;
}
CONVERT_EXTERN_OBJECT(ActionProfile) {
    auto primitive = mkPrimitive("ActionProfile");
    return primitive;
}
CONVERT_EXTERN_OBJECT(ActionSelector) {
    auto primitive = mkPrimitive("ActionSelector");
    return primitive;
}
CONVERT_EXTERN_OBJECT(Digest) {
    auto primitive = mkPrimitive("Digest");
    return primitive;
}

CONVERT_EXTERN_INSTANCE(Hash) {
    // ctxt->json->
}
CONVERT_EXTERN_INSTANCE(Checksum) {
    // ctxt->json
}
CONVERT_EXTERN_INSTANCE(InternetChecksum) {
    // ctxt->json
}
CONVERT_EXTERN_INSTANCE(Counter) {
    // ctxt->json->counters
}
CONVERT_EXTERN_INSTANCE(DirectCounter) {
    // ctxt->json->counters
}
CONVERT_EXTERN_INSTANCE(Meter) {
    // ctxt->json->meter_arrays
}
CONVERT_EXTERN_INSTANCE(DirectMeter) {
    // ctxt->json->meter_arrays
}
CONVERT_EXTERN_INSTANCE(Register) {
    // ctxt->json->registers
}
CONVERT_EXTERN_INSTANCE(Random) {
    // ctxt->json->
}
CONVERT_EXTERN_INSTANCE(ActionProfile) {
    // ctxt->action_profiles
}
CONVERT_EXTERN_INSTANCE(ActionSelector) {
    // ctxt->action_profiles
}
CONVERT_EXTERN_INSTANCE(Digest) {
    // ctxt->json
}

}  // namespace BMV2
