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
#include "sexp.h"
#include "lib/stringify.h"

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

    auto convertToDpdk = new ConvertToDpdkProgram(structure);
    PassManager toAsm = {
        new BMV2::DiscoverStructure(&structure),
        new BMV2::InspectPsaProgram(refMap, typeMap, &structure),
        // convert to assembly program
        convertToDpdk,
    };

    program->apply(toAsm);
    dpdk_program = convertToDpdk->getDpdkProgram();
    if (!dpdk_program) return;
    // additional passes to optimize DPDK assembly
    // PassManager optimizeAsm = { }
    //program->apply(DumpAsm());
}

void PsaSwitchBackend::codegen(std::ostream& out) const {
    out << dpdk_program << std::endl;
}

const IR::DpdkAsmStatement* ConvertToDpdkProgram::createListStatement(cstring name,
        std::initializer_list<IR::IndexedVector<IR::DpdkAsmStatement>> list) {

    auto stmts = new IR::IndexedVector<IR::DpdkAsmStatement>();
    for (auto l : list) {
        stmts->append(l);
    }
    return new IR::DpdkListStatement(name, *stmts);
}

const IR::DpdkAsmProgram* ConvertToDpdkProgram::create() {
    IR::IndexedVector<IR::DpdkHeaderType> headerType;
    for (auto kv : structure.header_types) {
        auto h = kv.second;
        auto ht = new IR::DpdkHeaderType(h->srcInfo, h->name, h->annotations, h->fields);
        headerType.push_back(ht);
    }
    IR::IndexedVector<IR::DpdkStructType> structType;
    for (auto kv : structure.metadata_types) {
        auto s = kv.second;
        auto st = new IR::DpdkStructType(s->srcInfo, s->name, s->annotations, s->fields);
        structType.push_back(st);
    }
    IR::IndexedVector<IR::DpdkAsmStatement> statements;
    auto ingress_parser_converter = new ConvertToDpdkParser();
    auto egress_parser_converter = new ConvertToDpdkParser();
    for (auto kv : structure.parsers) {
        if (kv.first == "ingress")
            kv.second->apply(*ingress_parser_converter);
        else if (kv.first == "egress")
            kv.second->apply(*egress_parser_converter);
        else
            BUG("Unknown parser %s", kv.second->name);
    }
    auto ingress_converter = new ConvertToDpdkControl();
    auto egress_converter = new ConvertToDpdkControl();
    for (auto kv : structure.pipelines) {
        if (kv.first == "ingress")
            kv.second->apply(*ingress_converter);
        else if (kv.first == "egress")
            kv.second->apply(*egress_converter);
        else
            BUG("Unknown control block %s", kv.second->name);
    }
    auto ingress_deparser_converter = new ConvertToDpdkControl();
    auto egress_deparser_converter = new ConvertToDpdkControl();
    for (auto kv : structure.deparsers) {
        if (kv.first == "ingress")
            kv.second->apply(*ingress_deparser_converter);
        else if (kv.first == "egress")
            kv.second->apply(*egress_deparser_converter);
        else
            BUG("Unknown deparser block %s", kv.second->name);
    }
    statements.append(ingress_converter->getActions());
    statements.append(ingress_converter->getTables());

    // ingress processing
    auto ingress_statements = createListStatement("ingress",
            { ingress_parser_converter->getInstructions(),
              ingress_converter->getInstructions(),
              ingress_deparser_converter->getInstructions() });
    statements.push_back(ingress_statements);

    statements.append(egress_converter->getActions());
    statements.append(egress_converter->getTables());

    // egress processing
    auto egress_statements = createListStatement("egress",
            { egress_parser_converter->getInstructions(),
              egress_converter->getInstructions(),
              egress_deparser_converter->getInstructions() });
    statements.push_back(egress_statements);

    return new IR::DpdkAsmProgram(headerType, structType, statements);
}

const IR::Node* ConvertToDpdkProgram::preorder(IR::P4Program* prog) {
    dpdk_program = create();
    return prog;
}

bool ConvertToDpdkParser::preorder(const IR::P4Parser* p) {
    return true;
}

bool ConvertToDpdkParser::preorder(const IR::ParserState* s) {
    return true;
}

bool ConvertToDpdkControl::preorder(const IR::P4Action* a) {
    return true;
}

bool ConvertToDpdkControl::preorder(const IR::IfStatement* stmt) {
    auto name = Util::printf_format("tmp_%d", next_reg_id++);
    auto cond = new IR::DpdkMovStatement(new IR::PathExpression(IR::ID(name)), stmt->condition);
    add_inst(cond);
    //auto true_label  = Util::printf_format("label_%d", next_label_id++);
    //auto false_label = Util::printf_format("label_%d", next_label_id++);
    //auto end_label = Util::printf_format("label_%d", next_label_id++);
    //add_inst(new IR::DpdkJmpStatement(true_label));
    //add_inst(new IR::DpdkJmpStatement(false_label));
    //add_inst(new IR::DpdkLabelStatement(true_label));
    //visit(stmt->ifTrue);
    //add_inst(new IR::DpdkLabelStatement(false_label));
    //visit(stmt->ifFalse);
    return false;
}

bool ConvertToDpdkControl::preorder(const IR::MethodCallStatement* stmt) {
    //add_inst(new IR::DpdkExternObjStatement(stmt->methodCall));
    return false;
}

bool ConvertToDpdkControl::preorder(const IR::AssignmentStatement* stmt) {
    //add_inst(new IR::DpdkMovStatement(stmt->left, stmt->right));
    return true;
}

bool ConvertToDpdkControl::preorder(const IR::ReturnStatement* stmt) {
    return true;
}

}  // namespace DPDK
