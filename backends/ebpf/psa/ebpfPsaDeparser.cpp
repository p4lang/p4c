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
#include "ebpfPsaDeparser.h"
#include "ebpfPipeline.h"

namespace EBPF {

DeparserBodyTranslatorPSA::DeparserBodyTranslatorPSA(const EBPFDeparserPSA *deparser) :
        CodeGenInspector(deparser->program->refMap, deparser->program->typeMap),
        DeparserBodyTranslator(deparser) {
    setName("DeparserBodyTranslatorPSA");
}

void DeparserBodyTranslatorPSA::processFunction(const P4::ExternFunction *function) {
    auto dprs = dynamic_cast<const EBPFDeparserPSA*>(deparser);
    if (function->method->name.name == "psa_resubmit") {
        builder->appendFormat("(!%s->drop && %s->resubmit)",
                              dprs->istd->name.name, dprs->istd->name.name);
    }
}

void EBPFDeparserPSA::emitDeclaration(CodeBuilder* builder, const IR::Declaration* decl) {
    // placeholder for handling checksums
    EBPFDeparser::emitDeclaration(builder, decl);
}

// =====================IngressDeparserPSA=============================
bool IngressDeparserPSA::build() {
    auto pl = controlBlock->container->type->applyParams;

    if (pl->size() != 7) {
        ::error(ErrorType::ERR_EXPECTED,
                "Expected ingress deparser to have exactly 7 parameters");
        return false;
    }

    auto it = pl->parameters.begin();
    packet_out = *it;
    headers = *(it + 4);
    user_metadata = *(it + 5);
    resubmit_meta = *(it + 2);

    auto ht = program->typeMap->getType(headers);
    if (ht == nullptr) {
        return false;
    }
    headerType = EBPFTypeFactory::instance->create(ht);

    return true;
}

// =====================EgressDeparserPSA=============================
bool EgressDeparserPSA::build() {
    auto pl = controlBlock->container->type->applyParams;
    auto it = pl->parameters.begin();
    packet_out = *it;
    headers = *(it + 3);

    auto ht = program->typeMap->getType(headers);
    if (ht == nullptr) {
        return false;
    }
    headerType = EBPFTypeFactory::instance->create(ht);

    return true;
}

// =====================TCIngressDeparserPSA=============================
/*
 * PreDeparser for Ingress pipeline implements:
 * - packet cloning (using clone sessions)
 * - early packet drop
 * - resubmission
 */
void TCIngressDeparserPSA::emitPreDeparser(CodeBuilder *builder) {
    builder->emitIndent();

    builder->newline();
    builder->emitIndent();

    // clone support
    builder->appendFormat("if (%s->clone) ", istd->name.name);
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("do_packet_clones(%s, &clone_session_tbl, %s->clone_session_id,"
                          " CLONE_I2E, 1);", program->model.CPacketName.str(), istd->name.name);
    builder->newline();
    builder->blockEnd(true);

    // early drop
    builder->emitIndent();
    builder->appendFormat("if (%s->drop) ", istd->name.name);
    builder->blockStart();
    builder->target->emitTraceMessage(builder, "PreDeparser: dropping packet..");
    builder->emitIndent();
    builder->appendFormat("return %s;\n", builder->target->abortReturnCode().c_str());
    builder->blockEnd(true);

    // if packet should be resubmitted, we skip deparser
    builder->emitIndent();
    builder->appendFormat("if (%s->resubmit) ", istd->name.name);
    builder->blockStart();
    builder->target->emitTraceMessage(builder, "PreDeparser: resubmitting packet, "
                                               "skipping deparser..");
    builder->emitIndent();
    CHECK_NULL(program);
    auto pipeline = dynamic_cast<const EBPFPipeline*>(program);
    builder->appendFormat("%s->packet_path = RESUBMIT;",
                          pipeline->compilerGlobalMetadata);
    builder->newline();
    builder->emitIndent();
    builder->appendLine("return TC_ACT_UNSPEC;");
    builder->blockEnd(true);
}
}  // namespace EBPF
