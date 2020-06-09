/*
Copyright 2020 Intel Corp.

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
#include "backend.h"
#include "backends/bmv2/psa_switch/psaSwitch.h"
#include "ir/ir.h"

namespace DPDK {

void PsaSwitchBackend::convert(const IR::ToplevelBlock* tlb) {
    CHECK_NULL(tlb);
    BMV2::PsaProgramStructure structure(refMap, typeMap);
    auto parsePsaArch = new BMV2::ParsePsaArchitecture(&structure);
    auto main = tlb->getMain();
    if (!main) return;

    if (main->type->name != "PSA_Switch")
        ::warning(ErrorType::WARN_INVALID, "%1%: the main package should be called PSA_Switch"
                  "; are you using the wrong architecture?", main->type->name);

    main->apply(*parsePsaArch);

    auto evaluator = new P4::EvaluatorPass(refMap, typeMap);
    auto program = tlb->getProgram();
    PassManager simplify = {
        /* TODO */
        // new RenameUserMetadata(refMap, userMetaType, userMetaName),
        new P4::ClearTypeMap(typeMap),  // because the user metadata type has changed
        new P4::SynthesizeActions(refMap, typeMap,
                new BMV2::SkipControls(&structure.non_pipeline_controls)),
        new P4::MoveActionsToTables(refMap, typeMap),
        new P4::TypeChecking(refMap, typeMap),
        new P4::SimplifyControlFlow(refMap, typeMap),
        new BMV2::LowerExpressions(typeMap),
        new P4::ConstantFolding(refMap, typeMap, false),
        new P4::TypeChecking(refMap, typeMap),
        new BMV2::RemoveComplexExpressions(refMap, typeMap,
                new BMV2::ProcessControls(&structure.pipeline_controls)),
        new P4::SimplifyControlFlow(refMap, typeMap),
        new P4::RemoveAllUnusedDeclarations(refMap),
        // Converts the DAG into a TREE (at least for expressions)
        // This is important later for conversion to JSON.
        evaluator,
        new VisitFunctor([this, evaluator, structure]() {
            toplevel = evaluator->getToplevelBlock(); }),
    };
    auto hook = options.getDebugHook();
    simplify.addDebugHook(hook);
    program->apply(simplify);

    // map IR node to compile-time allocated resource blocks.
    toplevel->apply(*new BMV2::BuildResourceMap(&structure.resourceMap));

    main = toplevel->getMain();
    if (!main) return;  // no main
    main->apply(*parsePsaArch);
    program = toplevel->getProgram();

    PassManager toAsm = {
        new BMV2::DiscoverStructure(&structure),
        new BMV2::InspectPsaProgram(refMap, typeMap, &structure),
        new BMV2::ConvertPsaToJson(refMap, typeMap, toplevel, json, &structure),
        new DumpAsm(&structure, json, &asm_),
    };

    program->apply(toAsm);
}

Visitor::profile_t DumpAsm::init_apply(const IR::Node* node) {
    for (auto p : *json->parsers) {
        // back from json is difficult
        std::cout << p->to<Util::JsonObject>()->get("name")->toString() << std::endl;
        parsers.emplace(p->to<Util::JsonObject>()->get("name")->toString(), p->to<Util::JsonObject>());
    }
    return Inspector::init_apply(node);
}

void PsaSwitchBackend::codegen(std::ostream& out) const {
    out << asm_ << std::endl;
}

bool DumpAsm::preorder(const IR::P4Parser* p) {
    LOG1("parser " << p->name.name);
    if (parsers.count(p->name.name)) {
        LOG1(p->name.name);
    }
    return true;
}

bool DumpAsm::preorder(const IR::P4Control* c) {
    //
    return true;
}

void DumpAsm::end_apply() {
    for (auto h : structure->header_types) {
        // translate to asm syntax
        asm_->header_config.push_back(h.first);
    }
    for (auto m : structure->metadata_types) {
        asm_->metadata_config.push_back(m.first);
    }
    // visit P4::Control to collect tables
    for (auto p : structure->pipelines) {
    }
    // visit P4::Parser to collect parsers
    for (auto p : structure->parsers) {
    }
    // visit P4::Control to collect deparsers
    for (auto d : structure->deparsers) {
    }
}

// dump asm object to text
std::ostream& operator<<(std::ostream& out, const Asm& asm_) {
    out << "# struct " << std::endl;
    for (auto s : asm_.metadata_config)
        out << s << std::endl;;
    out << "# header " << std::endl;
    for (auto h : asm_.header_config)
        out << h << std::endl;
    out << "# table " << std::endl;
    for (auto t : asm_.table_config)
        out << t << std::endl;
    out << "# action " << std::endl;
    for (auto a : asm_.action_config)
        out << a << std::endl;
    out << "# instr " << std::endl;
    for (auto i : asm_.instr_config)
        out << i << std::endl;
    return out;
}

}  // namespace DPDK
