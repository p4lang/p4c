// SPDX-FileCopyrightText: 2022 Open Networking Foundation
// SPDX-FileCopyrightText: 2022 Orange
//
// SPDX-License-Identifier: Apache-2.0
#include "ebpfPsaParser.h"

#include "backends/ebpf/ebpfType.h"
#include "backends/ebpf/psa/ebpfPipeline.h"
#include "frontends/p4/enumInstance.h"

namespace P4::EBPF {

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
}  // namespace P4::EBPF
