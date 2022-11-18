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

DeparserBodyTranslatorPSA::DeparserBodyTranslatorPSA(const EBPFDeparserPSA* deparser)
    : CodeGenInspector(deparser->program->refMap, deparser->program->typeMap),
      DeparserBodyTranslator(deparser) {
    setName("DeparserBodyTranslatorPSA");
}

void DeparserBodyTranslatorPSA::processFunction(const P4::ExternFunction* function) {
    auto dprs = deparser->to<EBPFDeparserPSA>();
    CHECK_NULL(dprs);
    if (function->method->name.name == "psa_resubmit") {
        builder->appendFormat("(!%s->drop && %s->resubmit)", dprs->istd->name.name,
                              dprs->istd->name.name);
    }
}

void DeparserBodyTranslatorPSA::processMethod(const P4::ExternMethod* method) {
    auto dprs = deparser->to<EBPFDeparserPSA>();
    CHECK_NULL(dprs);
    auto externName = method->originalExternType->name.name;
    if (externName == "Checksum" || externName == "InternetChecksum") {
        auto instance = method->object->getName().name;
        auto methodName = method->method->getName().name;
        dprs->getChecksum(instance)->processMethod(builder, methodName, method->expr, this);
        return;
    } else if (method->method->name.name == "pack") {
        // Emit digest pack method
        auto obj = method->object;
        auto di = obj->to<IR::Declaration_Instance>();
        cstring digestMapName = EBPFObject::externalName(di);
        auto arg = method->expr->arguments->front();
        builder->appendFormat("bpf_map_push_elem(&%s, &", digestMapName);
        this->visit(arg);
        builder->appendFormat(", BPF_EXIST)");
        return;
    }

    DeparserBodyTranslator::processMethod(method);
}

void EBPFDeparserPSA::emitDigestInstances(CodeBuilder* builder) const {
    for (auto digest : digests) {
        builder->appendFormat("REGISTER_TABLE_NO_KEY_TYPE(%s, %s, 0, ", digest.first,
                              "BPF_MAP_TYPE_QUEUE");
        auto type = EBPFTypeFactory::instance->create(digest.second->to<IR::Type_Type>()->type);
        type->declare(builder, "", false);
        builder->appendFormat(", %d)", maxDigestQueueSize);
        builder->newline();
    }
}

void EBPFDeparserPSA::emitDeclaration(CodeBuilder* builder, const IR::Declaration* decl) {
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
void TCIngressDeparserPSA::emitPreDeparser(CodeBuilder* builder) {
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
    auto pipeline = dynamic_cast<const EBPFPipeline*>(program);
    builder->appendFormat("%s->packet_path = RESUBMIT;", pipeline->compilerGlobalMetadata);
    builder->newline();
    builder->emitIndent();
    builder->appendLine("return TC_ACT_UNSPEC;");
    builder->blockEnd(true);
}
}  // namespace EBPF
