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

void
Backend::process(const IR::ToplevelBlock* tlb, BMV2Options& options) {
    CHECK_NULL(tlb);
    auto evaluator = new P4::EvaluatorPass(refMap, typeMap);
    if (tlb->getMain() == nullptr)
        return;  // no main

    if (options.arch != Target::UNKNOWN)
        target = options.arch;

    if (target == Target::SIMPLE) {
        simpleSwitch->setPipelineControls(tlb, &pipeline_controls, &pipeline_namemap);
        simpleSwitch->setNonPipelineControls(tlb, &non_pipeline_controls);
        simpleSwitch->setUpdateChecksumControls(tlb, &update_checksum_controls);
        simpleSwitch->setVerifyChecksumControls(tlb, &verify_checksum_controls);
        simpleSwitch->setDeparserControls(tlb, &deparser_controls);
    } else if (target == Target::PORTABLE) {
        P4C_UNIMPLEMENTED("PSA architecture is not yet implemented");
    }

    // These passes are logically bmv2-specific
    addPasses({
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
    jsonTop.emplace("errors", json->errors);
    jsonTop.emplace("enums", json->enums);
    jsonTop.emplace("parsers", json->parsers);
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

    (void)toplevel->apply(ConvertGlobals(this));
    main->apply(codegen_passes);
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

}  // namespace BMV2
