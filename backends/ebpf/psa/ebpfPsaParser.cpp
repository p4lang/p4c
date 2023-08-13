/*
Copyright 2022-present Orange
Copyright 2022-present Open Networking Foundation

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
#include "ebpfPsaParser.h"

#include "backends/ebpf/ebpfObject.h"
#include "backends/ebpf/ebpfParser.h"
#include "backends/ebpf/psa/ebpfPipeline.h"
#include "backends/ebpf/psa/externs/ebpfPsaChecksum.h"
#include "backends/ebpf/target.h"
#include "ir/declaration.h"
#include "ir/id.h"

namespace EBPF {

void PsaStateTranslationVisitor::processMethod(const P4::ExternMethod *ext) {
    auto externName = ext->originalExternType->name.name;

    if (externName == "Checksum" || externName == "InternetChecksum") {
        auto instance = ext->object->getName().name;
        auto method = ext->method->getName().name;
        parser->getChecksum(instance)->processMethod(builder, method, ext->expr, this);
        return;
    }

    StateTranslationVisitor::processMethod(ext);
}

// =====================EBPFPsaParser=============================
EBPFPsaParser::EBPFPsaParser(const EBPFProgram *program, const IR::ParserBlock *block,
                             const P4::TypeMap *typeMap)
    : EBPFParser(program, block, typeMap), inputMetadata(nullptr) {
    visitor = new PsaStateTranslationVisitor(program->refMap, program->typeMap, this);
}

void EBPFPsaParser::emit(CodeBuilder *builder) {
    builder->emitIndent();
    builder->blockStart();
    emitParserInputMetadata(builder);
    EBPFParser::emit(builder);
    builder->blockEnd(true);
}

void EBPFPsaParser::emitParserInputMetadata(CodeBuilder *builder) {
    bool isIngress = program->is<EBPFIngressPipeline>();

    builder->emitIndent();
    if (isIngress) {
        builder->append("struct psa_ingress_parser_input_metadata_t ");
    } else {
        builder->append("struct psa_egress_parser_input_metadata_t ");
    }
    builder->append(inputMetadata->name.name);
    builder->append(" = ");
    builder->blockStart();

    builder->emitIndent();
    if (isIngress) {
        builder->append(".ingress_port = ");
    } else {
        builder->append(".egress_port = ");
    }
    builder->append(program->to<EBPFPipeline>()->inputPortVar);
    builder->appendLine(",");

    builder->emitIndent();
    builder->appendLine(".packet_path = compiler_meta__->packet_path,");

    builder->blockEnd(false);
    builder->endOfStatement(true);
}

void EBPFPsaParser::emitDeclaration(CodeBuilder *builder, const IR::Declaration *decl) {
    if (auto di = decl->to<IR::Declaration_Instance>()) {
        cstring name = di->name.name;
        if (EBPFObject::getSpecializedTypeName(di) == "Checksum") {
            auto instance = new EBPFChecksumPSA(program, di, name);
            checksums.emplace(name, instance);
            instance->emitVariables(builder);
            return;
        } else if (EBPFObject::getTypeName(di) == "InternetChecksum") {
            auto instance = new EBPFInternetChecksumPSA(program, di, name);
            checksums.emplace(name, instance);
            instance->emitVariables(builder);
            return;
        }
    }

    EBPFParser::emitDeclaration(builder, decl);
}

void EBPFPsaParser::emitRejectState(CodeBuilder *builder) {
    builder->emitIndent();
    builder->appendFormat("if (%s == 0) ", program->errorVar.c_str());
    builder->blockStart();
    builder->target->emitTraceMessage(
        builder, "Parser: Explicit transition to reject state, dropping packet..");
    builder->emitIndent();
    builder->appendFormat("return %s", builder->target->abortReturnCode().c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);

    builder->emitIndent();
    builder->appendFormat("goto %s", IR::ParserState::accept.c_str());
    builder->endOfStatement(true);
}
}  // namespace EBPF
