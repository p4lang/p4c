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

#include <string>
#include <utility>
#include <vector>

#include "backends/ebpf/ebpfDeparser.h"
#include "backends/ebpf/ebpfModel.h"
#include "backends/ebpf/ebpfObject.h"
#include "backends/ebpf/ebpfOptions.h"
#include "backends/ebpf/ebpfType.h"
#include "backends/ebpf/psa/externs/ebpfPsaChecksum.h"
#include "backends/ebpf/target.h"
#include "ebpfPipeline.h"
#include "frontends/common/model.h"
#include "frontends/p4/typeMap.h"
#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/null.h"

namespace EBPF {

DeparserBodyTranslatorPSA::DeparserBodyTranslatorPSA(const EBPFDeparserPSA *deparser)
    : CodeGenInspector(deparser->program->refMap, deparser->program->typeMap),
      DeparserBodyTranslator(deparser) {
    setName("DeparserBodyTranslatorPSA");
}

void DeparserBodyTranslatorPSA::processFunction(const P4::ExternFunction *function) {
    auto dprs = deparser->to<EBPFDeparserPSA>();
    CHECK_NULL(dprs);
    if (function->method->name.name == "psa_resubmit") {
        builder->appendFormat("(!%s->drop && %s->resubmit)", dprs->istd->name.name,
                              dprs->istd->name.name);
    }
}

void DeparserBodyTranslatorPSA::processMethod(const P4::ExternMethod *method) {
    auto dprs = deparser->to<EBPFDeparserPSA>();
    CHECK_NULL(dprs);
    auto externName = method->originalExternType->name.name;
    auto instance = method->object->getName().name;
    auto methodName = method->method->getName().name;
    cstring externalName = EBPFObject::externalName(method->object);

    if (externName == "Checksum" || externName == "InternetChecksum") {
        dprs->getChecksum(instance)->processMethod(builder, methodName, method->expr, this);
        return;
    }

    if (externName == "Digest") {
        dprs->getDigest(externalName)->processMethod(builder, methodName, method->expr, this);
        return;
    }

    DeparserBodyTranslator::processMethod(method);
}

void EBPFDeparserPSA::emitTypes(CodeBuilder *builder) const {
    for (auto digest : digests) {
        digest.second->emitTypes(builder);
    }
}

void EBPFDeparserPSA::emitDigestInstances(CodeBuilder *builder) const {
    for (auto digest : digests) {
        digest.second->emitInstance(builder);
    }
}

void EBPFDeparserPSA::emitDeclaration(CodeBuilder *builder, const IR::Declaration *decl) {
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

    EBPFDeparser::emitDeclaration(builder, decl);
}

// =====================IngressDeparserPSA=============================
bool IngressDeparserPSA::build() {
    auto pl = controlBlock->container->type->applyParams;

    if (pl->size() != 7) {
        ::error(ErrorType::ERR_EXPECTED, "Expected ingress deparser to have exactly 7 parameters");
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
    builder->appendFormat(
        "do_packet_clones(%s, &clone_session_tbl, %s->clone_session_id,"
        " CLONE_I2E, 1);",
        program->model.CPacketName.str(), istd->name.name);
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
    builder->target->emitTraceMessage(builder,
                                      "PreDeparser: resubmitting packet, "
                                      "skipping deparser..");
    builder->emitIndent();
    CHECK_NULL(program);
    auto pipeline = dynamic_cast<const EBPFPipeline *>(program);
    builder->appendFormat("%s->packet_path = RESUBMIT;", pipeline->compilerGlobalMetadata);
    builder->newline();
    builder->emitIndent();
    builder->appendLine("return TC_ACT_UNSPEC;");
    builder->blockEnd(true);
}

// =====================TCIngressDeparserForTrafficManagerPSA===========
void TCIngressDeparserForTrafficManagerPSA::emitPreDeparser(CodeBuilder *builder) {
    // clone support
    builder->emitIndent();
    builder->appendFormat("if (%s.clone) ", this->istd->name.name);
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat(
        "do_packet_clones(%s, &clone_session_tbl, %s.clone_session_id,"
        " CLONE_I2E, 1);",
        program->model.CPacketName.str(), this->istd->name.name);
    builder->newline();
    builder->blockEnd(true);
}

// =====================XDPIngressDeparserPSA=============================
void XDPIngressDeparserPSA::emitPreDeparser(CodeBuilder *builder) {
    builder->emitIndent();
    // Perform early multicast detection; if multicast is invoked, a packet will be
    // passed up anyway, so we can do deparsing entirely in TC.
    // In the NTK path follow any replication path, but it has to be checked if packet is not
    // resubmitted or multicasted because this check is not at the end of deparser.
    cstring conditionSendToTC =
        "if (%s->clone || %s->multicast_group != 0 ||"
        " (!%s->drop && %s->egress_port == 0 && !%s->resubmit && %s->multicast_group == 0)) ";
    conditionSendToTC = conditionSendToTC.replace("%s", istd->name.name);
    builder->append(conditionSendToTC);
    builder->blockStart();
    builder->emitIndent();
    builder->appendLine("struct xdp2tc_metadata xdp2tc_md = {};");
    builder->emitIndent();
    builder->appendFormat("xdp2tc_md.headers = *%s", this->parserHeaders->name.name);

    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("xdp2tc_md.ostd = *%s", this->istd->name.name);
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("xdp2tc_md.packetOffsetInBits = %s", this->program->offsetVar);
    builder->endOfStatement(true);
    builder->append(
        "    void *data = (void *)(long)skb->data;\n"
        "    void *data_end = (void *)(long)skb->data_end;\n"
        "    struct ethhdr *eth = data;\n"
        "    if ((void *)((struct ethhdr *) eth + 1) > data_end) {\n"
        "        return XDP_ABORTED;\n"
        "    }\n"
        "    xdp2tc_md.pkt_ether_type = eth->h_proto;\n"
        "    eth->h_proto = bpf_htons(0x0800);\n");
    if (program->options.xdp2tcMode == XDP2TC_HEAD) {
        builder->emitIndent();
        builder->appendFormat("int ret = bpf_xdp_adjust_head(%s, -(int)%s)",
                              program->model.CPacketName.str(), "sizeof(struct xdp2tc_metadata)");
        builder->endOfStatement(true);
        builder->emitIndent();
        builder->append("if (ret) ");
        builder->blockStart();
        builder->target->emitTraceMessage(builder, "Deparser: failed to push XDP2TC metadata");
        builder->emitIndent();
        builder->appendFormat("return %s;", builder->target->abortReturnCode().c_str());
        builder->newline();
        builder->blockEnd(true);
        builder->append(
            "    data = (void *)(long)skb->data;\n"
            "    data_end = (void *)(long)skb->data_end;\n"
            "    if (((char *) data + 14 + sizeof(struct xdp2tc_metadata)) > (char *) data_end) {\n"
            "        return XDP_ABORTED;\n"
            "    }\n");
        builder->emitIndent();
        builder->appendLine("__builtin_memmove(data, data + sizeof(struct xdp2tc_metadata), 14);");
        builder->emitIndent();
        builder->appendLine(
            "__builtin_memcpy(data + 14, "
            "&xdp2tc_md, sizeof(struct xdp2tc_metadata));");
    } else if (program->options.xdp2tcMode == XDP2TC_CPUMAP) {
        builder->emitIndent();
        builder->target->emitTableUpdate(builder, "xdp2tc_shared_map",
                                         this->program->zeroKey.c_str(), "xdp2tc_md");
        builder->newline();
    }
    builder->target->emitTraceMessage(builder, "Sending packet up to TC for cloning or to kernel");
    builder->emitIndent();
    builder->appendFormat("return %s", builder->target->forwardReturnCode());
    builder->endOfStatement(true);
    builder->blockEnd(true);
    builder->emitIndent();
    builder->appendFormat("if (%s->drop) ", istd->name.name, istd->name.name);
    builder->blockStart();
    builder->target->emitTraceMessage(builder, "PreDeparser: dropping packet..");
    builder->emitIndent();
    builder->appendFormat("return %s;\n", builder->target->abortReturnCode().c_str());
    builder->blockEnd(true);

    // if packet should be resubmitted, we skip deparser
    builder->emitIndent();
    builder->appendFormat("if (%s->resubmit) ", istd->name.name);
    builder->blockStart();
    builder->target->emitTraceMessage(builder,
                                      "PreDeparser: resubmitting packet, "
                                      "skipping deparser..");
    builder->emitIndent();
    builder->appendFormat("%s->packet_path = RESUBMIT;",
                          program->to<EBPFPipeline>()->compilerGlobalMetadata);
    builder->newline();
    builder->emitIndent();
    builder->appendLine("return -1;");
    builder->blockEnd(true);
}

// =====================XDPEgressDeparserPSA=============================
void XDPEgressDeparserPSA::emitPreDeparser(CodeBuilder *builder) {
    builder->emitIndent();
    builder->appendFormat("if (%s.drop) ", istd->name.name);
    builder->blockStart();
    builder->target->emitTraceMessage(builder, "PreDeparser: dropping packet..");
    builder->emitIndent();
    builder->appendFormat("return %s;\n", builder->target->abortReturnCode().c_str());
    builder->blockEnd(true);
}

}  // namespace EBPF
