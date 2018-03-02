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

#include "action.h"
#include "backend.h"
#include "control.h"
#include "deparser.h"
#include "errorcode.h"
#include "expression.h"
#include "extern.h"
#include "globals.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/unusedDeclarations.h"
#include "midend/actionSynthesis.h"
#include "midend/removeLeftSlices.h"
#include "lower.h"
#include "header.h"
#include "parser.h"
#include "JsonObjects.h"
#include "extractArchInfo.h"

namespace BMV2 {

/**
This class implements a policy suitable for the SynthesizeActions pass.
The policy is: do not synthesize actions for the controls whose names
are in the specified set.
For example, we expect that the code in the deparser will not use any
tables or actions.
*/
class SkipControls : public P4::ActionSynthesisPolicy {
    // set of controls where actions are not synthesized
    const std::set<cstring> *skip;

 public:
    explicit SkipControls(const std::set<cstring> *skip) : skip(skip) { CHECK_NULL(skip); }
    bool convert(const IR::P4Control* control) const {
        if (skip->find(control->name) != skip->end())
            return false;
        return true;
    }
};

/**
This class implements a policy suitable for the RemoveComplexExpression pass.
The policy is: only remove complex expression for the controls whose names
are in the specified set.
For example, we expect that the code in ingress and egress will have complex
expression removed.
*/
class ProcessControls : public BMV2::RemoveComplexExpressionsPolicy {
    const std::set<cstring> *process;

 public:
    explicit ProcessControls(const std::set<cstring> *process) : process(process) {
        CHECK_NULL(process);
    }
    bool convert(const IR::P4Control* control) const {
        if (process->find(control->name) != process->end())
            return true;
        return false;
    }
};

cstring Backend::jsonAssignment(const IR::Type* type, bool inParser) {
    if (!inParser && type->is<IR::Type_Varbits>())
        return "assign_VL";
    if (type->is<IR::Type_HeaderUnion>())
        return "assign_union";
    if (type->is<IR::Type_Header>())
        return "assign_header";
    if (auto ts = type->to<IR::Type_Stack>()) {
        auto et = ts->elementType;
        if (et->is<IR::Type_HeaderUnion>())
            return "assign_union_stack";
        else
            return "assign_header_stack";
    }
    if (inParser)
        // Unfortunately set can do some things that assign cannot,
        // e.g., handle lookahead on the RHS.
        return "set";
    else
        return "assign";
}

/**
This pass adds @name annotations to all fields of the user metadata
structure so that they do not clash with fields of the headers
structure.  This is necessary because both of them become global
objects in the output json.
*/
class RenameUserMetadata : public Transform {
    P4::ReferenceMap* refMap;
    const IR::Type_Struct* userMetaType;
    // Used as a prefix for the fields of the userMetadata structure
    // and also as a name for the userMetadata type clone.
    cstring namePrefix;

 public:
    RenameUserMetadata(P4::ReferenceMap* refMap,
        const IR::Type_Struct* userMetaType, cstring namePrefix):
            refMap(refMap), userMetaType(userMetaType), namePrefix(namePrefix)
    { setName("RenameUserMetadata"); CHECK_NULL(refMap); }

    const IR::Node* postorder(IR::Type_Struct* type) override {
        // Clone the user metadata type.  We want this type to be used
        // only for parameters.  For any other variables we will used
        // the clone we create.
        if (userMetaType != getOriginal())
            return type;

        auto vec = new IR::IndexedVector<IR::Node>();
        LOG2("Creating clone" << getOriginal());
        auto clone = type->clone();
        clone->name = namePrefix;
        vec->push_back(clone);

        // Rename all fields
        IR::IndexedVector<IR::StructField> fields;
        for (auto f : type->fields) {
            auto anno = f->getAnnotation(IR::Annotation::nameAnnotation);
            cstring suffix = "";
            if (anno != nullptr)
                suffix = IR::Annotation::getName(anno);
            if (suffix.startsWith(".")) {
                // We can't change the name of this field.
                // Hopefully the user knows what they are doing.
                fields.push_back(f->clone());
                continue;
            }

            if (!suffix.isNullOrEmpty())
                suffix = cstring(".") + suffix;
            else
                suffix = cstring(".") + f->name;
            cstring newName = namePrefix + suffix;
            auto stringLit = new IR::StringLiteral(newName);
            LOG2("Renaming " << f << " to " << newName);
            auto annos = f->annotations->addOrReplace(
                IR::Annotation::nameAnnotation, stringLit);
            auto field = new IR::StructField(f->srcInfo, f->name, annos, f->type);
            fields.push_back(field);
        }

        auto annotated = new IR::Type_Struct(
            type->srcInfo, type->name, type->annotations, std::move(fields));
        vec->push_back(annotated);
        return vec;
    }

    const IR::Node* preorder(IR::Type_Name* type) override {
        // Find any reference to the user metadata type that is used
        // (but not for parameters or the package instantiation)
        // and replace it with the cloned type.
        if (!findContext<IR::Declaration_Variable>())
            return type;
        auto decl = refMap->getDeclaration(type->path);
        if (decl == userMetaType)
            type->path = new IR::Path(type->path->srcInfo, IR::ID(type->path->srcInfo, namePrefix));
        LOG2("Replacing reference with " << type);
        return type;
    }
};

void
Backend::process(const IR::ToplevelBlock* tlb, BMV2Options& options) {
    CHECK_NULL(tlb);
    auto evaluator = new P4::EvaluatorPass(refMap, typeMap);
    if (tlb->getMain() == nullptr)
        return;  // no main

    if (options.arch != Target::UNKNOWN)
        target = options.arch;

    /// Declaration which introduces the user metadata.
    /// We expect this to be a struct type.
    const IR::Type_Struct* userMetaType = nullptr;
    cstring userMetaName = refMap->newName("userMetadata");
    if (target == Target::SIMPLE) {
        simpleSwitch->setPipelineControls(tlb, &pipeline_controls, &pipeline_namemap);
        simpleSwitch->setNonPipelineControls(tlb, &non_pipeline_controls);
        simpleSwitch->setComputeChecksumControls(tlb, &compute_checksum_controls);
        simpleSwitch->setVerifyChecksumControls(tlb, &verify_checksum_controls);
        simpleSwitch->setDeparserControls(tlb, &deparser_controls);

        // Find the user metadata declaration
        auto parser = simpleSwitch->getParser(tlb);
        auto params = parser->getApplyParameters();
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
    } else if (target == Target::PORTABLE) {
        P4C_UNIMPLEMENTED("PSA architecture is not yet implemented");
    }

    // These passes are logically bmv2-specific
    addPasses({
        new RenameUserMetadata(refMap, userMetaType, userMetaName),
        new P4::ClearTypeMap(typeMap),  // because the user metadata type has changed
        new P4::SynthesizeActions(refMap, typeMap, new SkipControls(&non_pipeline_controls)),
        new P4::MoveActionsToTables(refMap, typeMap),
        new P4::TypeChecking(refMap, typeMap),
        new P4::SimplifyControlFlow(refMap, typeMap),
        new LowerExpressions(typeMap),
        new P4::ConstantFolding(refMap, typeMap, false),
        new P4::TypeChecking(refMap, typeMap),
        new RemoveComplexExpressions(refMap, typeMap, new ProcessControls(&pipeline_controls)),
        new P4::SimplifyControlFlow(refMap, typeMap),
        new P4::RemoveAllUnusedDeclarations(refMap),
        new DiscoverStructure(&structure),
        new ErrorCodesVisitor(&errorCodesMap),
        new ExtractArchInfo(typeMap),
        evaluator,
        new VisitFunctor([this, evaluator]() { toplevel = evaluator->getToplevelBlock(); }),
    });
    tlb->getProgram()->apply(*this);
}

/// BMV2 Backend that takes the top level block and converts it to a JsonObject
/// that can be interpreted by the BMv2 simulator.
void Backend::convert(BMV2Options& options) {
    jsonTop.emplace("program", options.file);
    jsonTop.emplace("__meta__", json->meta);
    jsonTop.emplace("header_types", json->header_types);
    jsonTop.emplace("headers", json->headers);
    jsonTop.emplace("header_stacks", json->header_stacks);
    jsonTop.emplace("header_union_types", json->header_union_types);
    jsonTop.emplace("header_unions", json->header_unions);
    jsonTop.emplace("header_union_stacks", json->header_union_stacks);
    field_lists = mkArrayField(&jsonTop, "field_lists");
    // field list and learn list ids in bmv2 are not consistent with ids for
    // other objects: they need to start at 1 (not 0) since the id is also used
    // as a "flag" to indicate that a certain simple_switch primitive has been
    // called (e.g. resubmit or generate_digest)
    BMV2::nextId("field_lists");
    jsonTop.emplace("errors", json->errors);
    jsonTop.emplace("enums", json->enums);
    jsonTop.emplace("parsers", json->parsers);
    jsonTop.emplace("parse_vsets", json->parse_vsets);
    jsonTop.emplace("deparsers", json->deparsers);
    meter_arrays = mkArrayField(&jsonTop, "meter_arrays");
    counters = mkArrayField(&jsonTop, "counter_arrays");
    register_arrays = mkArrayField(&jsonTop, "register_arrays");
    jsonTop.emplace("calculations", json->calculations);
    learn_lists = mkArrayField(&jsonTop, "learn_lists");
    BMV2::nextId("learn_lists");
    jsonTop.emplace("actions", json->actions);
    jsonTop.emplace("pipelines", json->pipelines);
    jsonTop.emplace("checksums", json->checksums);
    force_arith = mkArrayField(&jsonTop, "force_arith");
    jsonTop.emplace("extern_instances", json->externs);
    jsonTop.emplace("field_aliases", json->field_aliases);

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

    /// generate error types
    for (const auto &p : errorCodesMap) {
        auto name = p.first->toString();
        auto type = p.second;
        json->add_error(name, type);
    }

    cstring scalarsName = refMap->newName("scalars");

    // This visitor is used in multiple passes to convert expression to json
    conv = new ExpressionConverter(this, scalarsName);

    // if (psa) tlb->apply(new ConvertExterns());
    PassManager codegen_passes = {
        new ConvertHeaders(this, scalarsName),
        new ConvertExterns(this),  // only run when target == PSA
        new ConvertParser(this),
        new ConvertActions(this),
        new ConvertControl(this),
        new ConvertDeparser(this),
    };

    codegen_passes.setName("CodeGen");
    CHECK_NULL(toplevel);
    auto main = toplevel->getMain();
    if (main == nullptr)
        return;

    main->apply(codegen_passes);
    (void)toplevel->apply(ConvertGlobals(this));
}

bool Backend::isStandardMetadataParameter(const IR::Parameter* param) {
    if (target == Target::SIMPLE) {
        auto parser = simpleSwitch->getParser(getToplevelBlock());
        auto params = parser->getApplyParameters();
        if (params->size() != 4) {
            simpleSwitch->modelError("%1%: Expected 4 parameter for parser", parser);
            return false;
        }
        if (params->parameters.at(3) == param)
            return true;

        auto ingress = simpleSwitch->getIngress(getToplevelBlock());
        params = ingress->getApplyParameters();
        if (params->size() != 3) {
            simpleSwitch->modelError("%1%: Expected 3 parameter for ingress", ingress);
            return false;
        }
        if (params->parameters.at(2) == param)
            return true;

        auto egress = simpleSwitch->getEgress(getToplevelBlock());
        params = egress->getApplyParameters();
        if (params->size() != 3) {
            simpleSwitch->modelError("%1%: Expected 3 parameter for egress", egress);
            return false;
        }
        if (params->parameters.at(2) == param)
            return true;

        return false;
    } else {
        P4C_UNIMPLEMENTED("PSA architecture is not yet implemented");
    }
}

bool Backend::isUserMetadataParameter(const IR::Parameter* param) {
    if (target == Target::SIMPLE) {
        auto parser = simpleSwitch->getParser(getToplevelBlock());
        auto params = parser->getApplyParameters();
        if (params->size() != 4) {
            simpleSwitch->modelError("%1%: Expected 4 parameters for parser", parser);
            return false;
        }
        if (params->parameters.at(2) == param)
            return true;

        auto ingress = simpleSwitch->getIngress(getToplevelBlock());
        params = ingress->getApplyParameters();
        if (params->size() != 3) {
            simpleSwitch->modelError("%1%: Expected 3 parameters for ingress", ingress);
            return false;
        }
        if (params->parameters.at(1) == param)
            return true;

        auto egress = simpleSwitch->getEgress(getToplevelBlock());
        params = egress->getApplyParameters();
        if (params->size() != 3) {
            simpleSwitch->modelError("%1%: Expected 3 parameters for egress", egress);
            return false;
        }
        if (params->parameters.at(1) == param)
            return true;

        auto update = simpleSwitch->getCompute(getToplevelBlock());
        params = update->getApplyParameters();
        if (params->size() != 2) {
            simpleSwitch->modelError("%1%: Expected 2 parameters for ComputeChecksum", update);
            return false;
        }
        if (params->parameters.at(1) == param)
            return true;

        auto verify = simpleSwitch->getVerify(getToplevelBlock());
        params = verify->getApplyParameters();
        if (params->size() != 2) {
            simpleSwitch->modelError("%1%: Expected 2 parameters for VerifyChecksum", update);
            return false;
        }
        if (params->parameters.at(1) == param)
            return true;

        return false;
    } else {
        P4C_UNIMPLEMENTED("PSA architecture is not yet implemented");
    }
}

}  // namespace BMV2
