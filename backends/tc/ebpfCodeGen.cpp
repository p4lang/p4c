/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#include "ebpfCodeGen.h"

namespace P4::TC {

DeparserBodyTranslatorPNA::DeparserBodyTranslatorPNA(const IngressDeparserPNA *deparser)
    : CodeGenInspector(deparser->program->refMap, deparser->program->typeMap),
      DeparserBodyTranslatorPSA(deparser) {
    setName("DeparserBodyTranslatorPNA");
}

// =====================PNAEbpfGenerator=============================
void PNAEbpfGenerator::emitPNAIncludes(EBPF::CodeBuilder *builder) const {
    builder->appendLine("#include <stdbool.h>");
    builder->appendLine("#include <linux/if_ether.h>");
    builder->appendLine("#include \"pna.h\"");
}

cstring PNAEbpfGenerator::getProgramName() const { return cstring(options.file.stem()); }

void PNAEbpfGenerator::emitPreamble(EBPF::CodeBuilder *builder) const {
    emitCommonPreamble(builder);
    builder->newline();
}

void PNAEbpfGenerator::emitCommonPreamble(EBPF::CodeBuilder *builder) const {
    builder->newline();
    builder->appendLine("#define EBPF_MASK(t, w) ((((t)(1)) << (w)) - (t)1)");
    builder->appendLine("#define BYTES(w) ((w) / 8)");
    builder->appendLine(
        "#define write_partial(a, w, s, v) do { *((u8*)a) = ((*((u8*)a)) "
        "& ~(EBPF_MASK(u8, w) << s)) | (v << s) ; } while (0)");
    builder->appendLine(
        "#define write_byte(base, offset, v) do { "
        "*(u8*)((base) + (offset)) = (v); "
        "} while (0)");
    builder->target->emitPreamble(builder);
}

void PNAEbpfGenerator::emitInternalStructures(EBPF::CodeBuilder *builder) const {
    builder->appendLine("struct p4tc_filter_fields p4tc_filter_fields;");
    builder->newline();
    builder->append(
        "struct internal_metadata {\n"
        "    __u16 pkt_ether_type;\n"
        "} __attribute__((aligned(4)));\n\n"
        "struct skb_aggregate {\n"
        "    struct p4tc_skb_meta_get get;\n"
        "    struct p4tc_skb_meta_set set;\n"
        "};\n");
    builder->newline();
}

/* Generate headers and structs in p4 prog */
void PNAEbpfGenerator::emitTypes(EBPF::CodeBuilder *builder) const {
    pipeline->parser->emitTypes(builder);
    pipeline->control->emitTableTypes(builder);
    pipeline->deparser->emitTypes(builder);
    builder->newline();
}

void PNAEbpfGenerator::emitGlobalHeadersMetadata(EBPF::CodeBuilder *builder) const {
    builder->append("struct hdr_md ");
    builder->blockStart();
    builder->emitIndent();

    pipeline->parser->headerType->declare(builder, "cpumap_hdr"_cs, false);
    builder->endOfStatement(true);
    builder->emitIndent();
    auto user_md_type = pipeline->typeMap->getType(pipeline->control->user_metadata);
    BUG_CHECK(user_md_type != nullptr, "cannot declare user metadata");
    auto userMetadataType = EBPF::EBPFTypeFactory::instance->create(user_md_type);
    userMetadataType->declare(builder, "cpumap_usermeta"_cs, false);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("unsigned %s", pipeline->offsetVar.c_str());
    builder->endOfStatement(true);

    // additional field to avoid compiler errors when both headers and user_metadata are empty.
    builder->emitIndent();
    builder->append("__u8 __hook");
    builder->endOfStatement(true);

    builder->blockEnd(false);
    builder->endOfStatement(true);
    builder->newline();
}

void PNAEbpfGenerator::emitP4TCFilterFields(EBPF::CodeBuilder *builder) const {
    builder->append("struct p4tc_filter_fields ");
    builder->blockStart();

    builder->emitIndent();
    builder->appendFormat("__u32 pipeid");
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("__u32 handle");
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("__u32 classid");
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("__u32 chain");
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("__u32 blockid");
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("__be16 proto");
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("__u16 prio");
    builder->endOfStatement(true);
    emitP4TCActionParam(builder);
    builder->blockEnd(false);
    builder->endOfStatement(true);
    builder->newline();
}

void PNAEbpfGenerator::emitP4TCActionParam(EBPF::CodeBuilder *builder) const {
    for (auto table : tcIR->tcPipeline->tableDefs) {
        if (table->isTcMayOverrideMiss) {
            cstring tblName = table->getTableName();
            cstring defaultActionName = table->defaultMissAction->getActionName();
            defaultActionName = defaultActionName.substr(
                defaultActionName.find('/') - defaultActionName.begin() + 1,
                defaultActionName.size());
            for (auto param : table->defaultMissActionParams) {
                cstring paramName = param->paramDetail->getParamName();
                cstring placeholder = tblName + "_" + defaultActionName + "_" + paramName;
                cstring paramDecl = param->paramDetail->getParamDecl(placeholder);
                builder->emitIndent();
                builder->append(paramDecl);
                builder->endOfStatement(true);
            }
        }
        if (table->isTcMayOverrideHit) {
            cstring tblName = table->getTableName();
            cstring defaultActionName = table->defaultHitAction->getActionName();
            defaultActionName = defaultActionName.substr(
                defaultActionName.find('/') - defaultActionName.begin() + 1,
                defaultActionName.size());
            for (auto param : table->defaultHitActionParams) {
                cstring paramName = param->paramDetail->getParamName();
                cstring placeholder = tblName + "_" + defaultActionName + "_" + paramName;
                cstring paramDecl = param->paramDetail->getParamDecl(placeholder);
                builder->emitIndent();
                builder->append(paramDecl);
                builder->endOfStatement(true);
            }
        }
    }
}

void PNAEbpfGenerator::emitPipelineInstances(EBPF::CodeBuilder *builder) const {
    pipeline->parser->emitValueSetInstances(builder);

    builder->target->emitTableDecl(builder, "hdr_md_cpumap"_cs, EBPF::TablePerCPUArray, "u32"_cs,
                                   "struct hdr_md"_cs, 2);
}

// =====================PNAArchTC=============================
void PNAArchTC::emit(EBPF::CodeBuilder *builder) const {
    /**
     * Structure of a C Post Parser program for PNA
     * 1. Automatically generated comment
     * 2. Includes
     * 3. Headers, structs
     * 4. TC Pipeline program for post-parser.
     */

    // 1. Automatically generated comment.
    // Note we use inherited function from EBPFProgram.
    xdp->emitGeneratedComment(builder);

    /*
     * 2. Include header file.
     */
    cstring headerFile = getProgramName() + "_parser.h";
    builder->appendFormat("#include \"%v\"", headerFile);
    builder->newline();
    /*
     * 3. Headers, structs
     */
    emitInternalStructures(builder);
    emitTypes(builder);

    /*
     * 4. TC Pipeline program for post-parser.
     */
    pipeline->emit(builder);

    builder->target->emitLicense(builder, pipeline->license);
}

void PNAArchTC::emitInstances(EBPF::CodeBuilder *builder) const {
    builder->appendLine("REGISTER_START()");

    if (options.xdp2tcMode == XDP2TC_CPUMAP) {
        builder->target->emitTableDecl(builder, "xdp2tc_cpumap"_cs, EBPF::TablePerCPUArray,
                                       "u32"_cs, "u16"_cs, 1);
    }

    emitPipelineInstances(builder);
    builder->appendLine("REGISTER_END()");
    builder->newline();
}

void PNAArchTC::emitGlobalFunctions(EBPF::CodeBuilder *builder) const {
    const char *code =
        "static inline u32 getPrimitive32(u8 *a, int size) {\n"
        "   if(size <= 16 || size > 24) {\n"
        "       bpf_printk(\"Invalid size.\");\n"
        "   }\n"
        "   return  ((((u32)a[2]) <<16) | (((u32)a[1]) << 8) | a[0]);\n"
        "}\n"
        "static inline u64 getPrimitive64(u8 *a, int size) {\n"
        "   if(size <= 32 || size > 56) {\n"
        "       bpf_printk(\"Invalid size.\");\n"
        "   }\n"
        "   if(size <= 40) {\n"
        "       return  ((((u64)a[4]) << 32) | (((u64)a[3]) << 24) | (((u64)a[2]) << 16) | "
        "(((u64)a[1]) << 8) | a[0]);\n"
        "   } else {\n"
        "       if(size <= 48) {\n"
        "           return  ((((u64)a[5]) << 40) | (((u64)a[4]) << 32) | (((u64)a[3]) << 24) | "
        "(((u64)a[2]) << 16) | (((u64)a[1]) << 8) | a[0]);\n"
        "       } else {\n"
        "           return  ((((u64)a[6]) << 48) | (((u64)a[5]) << 40) | (((u64)a[4]) << 32) | "
        "(((u64)a[3]) << 24) | (((u64)a[2]) << 16) | (((u64)a[1]) << 8) | a[0]);\n"
        "       }\n"
        "   }\n"
        "}\n"
        "static inline void storePrimitive32(u8 *a, int size, u32 value) {\n"
        "   if(size <= 16 || size > 24) {\n"
        "       bpf_printk(\"Invalid size.\");\n"
        "   }\n"
        "   a[0] = (u8)(value);\n"
        "   a[1] = (u8)(value >> 8);\n"
        "   a[2] = (u8)(value >> 16);\n"
        "}\n"
        "static inline void storePrimitive64(u8 *a, int size, u64 value) {\n"
        "   if(size <= 32 || size > 56) {\n"
        "       bpf_printk(\"Invalid size.\");\n"
        "   }\n"
        "   a[0] = (u8)(value);\n"
        "   a[1] = (u8)(value >> 8);\n"
        "   a[2] = (u8)(value >> 16);\n"
        "   a[3] = (u8)(value >> 24);\n"
        "   a[4] = (u8)(value >> 32);\n"
        "   if (size > 40) {\n"
        "       a[5] = (u8)(value >> 40);\n"
        "   }\n"
        "   if (size > 48) {\n"
        "       a[6] = (u8)(value >> 48);\n"
        "   }\n"
        "}\n";
    builder->appendLine(code);
}

void PNAArchTC::emitParser(EBPF::CodeBuilder *builder) const {
    /**
     * Structure of a C Parser program for PNA
     * 1. Automatically generated comment
     * 2. Includes
     * 3. BPF map definitions.
     * 4. TC Pipeline program for parser.
     */
    xdp->emitGeneratedComment(builder);
    cstring headerFile = getProgramName() + "_parser.h";
    builder->appendFormat("#include \"%v\"", headerFile);
    builder->newline();
    builder->newline();
    builder->appendLine("struct p4tc_filter_fields p4tc_filter_fields;");
    builder->newline();
    pipeline->name = "tc-parse"_cs;
    pipeline->sectionName = "p4tc/parse"_cs;
    pipeline->functionName = pipeline->name.replace('-', '_') + "_func";
    pipeline->emit(builder);
    builder->target->emitLicense(builder, pipeline->license);
}

void PNAArchTC::emitHeader(EBPF::CodeBuilder *builder) const {
    xdp->emitGeneratedComment(builder);
    builder->target->emitIncludes(builder);
    emitPNAIncludes(builder);
    emitPreamble(builder);
    for (auto type : ebpfTypes) {
        type->emit(builder);
    }
    PNAErrorCodesGen errorGen(builder);
    pipeline->program->apply(errorGen);
    emitGlobalHeadersMetadata(builder);
    emitP4TCFilterFields(builder);
    //  BPF map definitions.
    emitInstances(builder);
    emitGlobalFunctions(builder);
}

// =====================TCIngressPipelinePNA=============================
void TCIngressPipelinePNA::emit(EBPF::CodeBuilder *builder) {
    cstring msgStr, varStr;

    // firstly emit process() in-lined function and then the actual BPF section.
    builder->append("static __always_inline");
    builder->spc();
    // FIXME: use Target to generate metadata type
    cstring func_name = (name == "tc-parse") ? "run_parser"_cs : "process"_cs;
    builder->appendFormat(
        "int %v(%v *%s, %v %v *%v, "
        "struct pna_global_metadata *%v",
        func_name, builder->target->packetDescriptorType(), model.CPacketName.str(),
        parser->headerType->as<EBPF::EBPFStructType>().kind,
        parser->headerType->as<EBPF::EBPFStructType>().name, parser->headers->name,
        compilerGlobalMetadata);
    if (func_name == "process") builder->append(", struct skb_aggregate *sa");
    builder->append(")");
    builder->newline();

    builder->blockStart();

    emitCPUMAPHeadersInitializers(builder);
    emitLocalVariables(builder);

    builder->newline();
    emitUserMetadataInstance(builder);

    if (name == "tc-parse") {
        builder->newline();
        emitCPUMAPInitializers(builder);
        builder->newline();
        builder->emitIndent();
        builder->appendFormat("unsigned %s = 0;", offsetVar.c_str());
    } else {
        emitCPUMAPLookup(builder);
        builder->emitIndent();
        builder->append("if (!hdrMd)");
        builder->newline();
        builder->emitIndent();
        builder->emitIndent();
        builder->appendFormat("return %v;", dropReturnCode());
        builder->newline();
        builder->emitIndent();
        builder->appendFormat("unsigned %s = hdrMd->%s;", offsetVar.c_str(), offsetVar.c_str());
        builder->newline();
        builder->emitIndent();
        builder->appendFormat("%s = %s + BYTES(%s);", headerStartVar.c_str(),
                              packetStartVar.c_str(), offsetVar.c_str());
    }
    builder->newline();
    emitHeadersFromCPUMAP(builder);
    builder->newline();
    emitMetadataFromCPUMAP(builder);
    builder->newline();

    if (name == "tc-parse") {
        msgStr = absl::StrFormat(
            "%v parser: parsing new packet, input_port=%%d, path=%%d, "
            "pkt_len=%%d",
            sectionName);
        varStr = absl::StrFormat("%v->packet_path", compilerGlobalMetadata);
        builder->target->emitTraceMessage(builder, msgStr.c_str(), 3, inputPortVar.c_str(), varStr,
                                          lengthVar.c_str());

        // PARSER
        parser->emit(builder);
        builder->newline();

        builder->emitIndent();
        builder->append(IR::ParserState::accept);
        builder->append(":");
        builder->newline();
    }

    if (name == "tc-ingress") {
        // CONTROL
        builder->blockStart();
        msgStr = absl::StrFormat("%v control: packet processing started", sectionName);
        builder->target->emitTraceMessage(builder, msgStr.c_str());
        ((EBPFControlPNA *)control)->emitExternDefinition(builder);
        control->emit(builder);
        builder->blockEnd(true);
        msgStr = absl::StrFormat("%v control: packet processing finished", sectionName);
        builder->target->emitTraceMessage(builder, msgStr.c_str());

        // DEPARSER
        builder->emitIndent();
        builder->blockStart();
        msgStr = absl::StrFormat("%v deparser: packet deparsing started", sectionName);
        builder->target->emitTraceMessage(builder, msgStr.c_str());
        deparser->emit(builder);
        msgStr = absl::StrFormat("%v deparser: packet deparsing finished", sectionName);
        builder->target->emitTraceMessage(builder, msgStr.c_str());
        builder->blockEnd(true);
    }

    if (name == "tc-parse") {
        builder->emitIndent();
        builder->appendFormat("hdrMd->%s = %s;", offsetVar.c_str(), offsetVar.c_str());
        builder->newline();
    }
    builder->emitIndent();
    builder->appendFormat("return %d;", actUnspecCode);
    builder->newline();
    builder->blockEnd(true);

    if (name == "tc-ingress") {
        builder->target->emitCodeSection(builder, sectionName);
        builder->emitIndent();
        builder->appendFormat("int %v(%v *%s)", functionName,
                              builder->target->packetDescriptorType(), model.CPacketName.str());
        builder->spc();

        builder->blockStart();

        builder->emitIndent();

        if ((dynamic_cast<EBPFControlPNA *>(control))->touched_skb_metadata ||
            (dynamic_cast<IngressDeparserPNA *>(deparser))->touched_skb_metadata) {
            builder->append("struct skb_aggregate skbstuff = {\n");
            builder->newline();
            builder->increaseIndent();
            builder->emitIndent();
            builder->appendFormat(
                ".get = { .bitmask = P4TC_SKB_META_GET_AT_INGRESS_BIT | "
                "P4TC_SKB_META_GET_FROM_INGRESS_BIT },");
            builder->newline();
            builder->emitIndent();
            builder->appendFormat(".set = { },");
            builder->newline();
            builder->decreaseIndent();
            builder->append("};\n");
        } else {
            builder->append("struct skb_aggregate skbstuff;\n");
        }

        emitGlobalMetadataInitializer(builder);

        emitHeaderInstances(builder);
        builder->newline();

        builder->emitIndent();
        builder->appendFormat("int ret = %d;", actUnspecCode);
        builder->newline();
        builder->emitIndent();
        builder->appendFormat("ret = %v(skb, ", func_name);

        builder->appendFormat("(%v %v *) %v, %v, &skbstuff);",
                              parser->headerType->as<EBPF::EBPFStructType>().kind,
                              parser->headerType->as<EBPF::EBPFStructType>().name,
                              parser->headers->name, compilerGlobalMetadata);

        builder->newline();
        builder->emitIndent();
        builder->appendFormat(
            "if (ret != %d) {\n"
            "        return ret;\n"
            "    }",
            actUnspecCode);
        builder->newline();

        this->emitTrafficManager(builder);

        builder->blockEnd(true);
    } else {
        builder->newline();
        builder->target->emitCodeSection(builder, sectionName);
        builder->emitIndent();
        builder->appendFormat("int %v(%v *%s)", functionName,
                              builder->target->packetDescriptorType(), model.CPacketName.str());
        builder->spc();

        builder->blockStart();

        builder->emitIndent();
        builder->appendFormat(
            "struct pna_global_metadata *%v = (struct pna_global_metadata *) skb->cb;",
            compilerGlobalMetadata);
        builder->newline();

        emitHeaderInstances(builder);
        builder->newline();

        builder->emitIndent();
        builder->appendFormat("int ret = %d;", actUnspecCode);
        builder->newline();
        builder->emitIndent();
        builder->appendFormat("ret = %v(skb, ", func_name);

        builder->appendFormat("(%v %v *) %v, %v);",
                              parser->headerType->as<EBPF::EBPFStructType>().kind,
                              parser->headerType->as<EBPF::EBPFStructType>().name,
                              parser->headers->name, compilerGlobalMetadata);

        builder->newline();
        builder->emitIndent();
        builder->appendFormat(
            "if (ret != %d) {\n"
            "        return ret;\n"
            "    }",
            actUnspecCode);
        builder->newline();
        builder->emitIndent();
        builder->appendFormat("return TC_ACT_PIPE;");
        builder->newline();
        builder->emitIndent();
        builder->blockEnd(true);
    }
}

void TCIngressPipelinePNA::emitGlobalMetadataInitializer(EBPF::CodeBuilder *builder) {
    builder->emitIndent();
    builder->appendFormat(
        "struct pna_global_metadata *%v = (struct pna_global_metadata *) skb->cb;",
        compilerGlobalMetadata);
    builder->newline();

    // Make sure drop and recirculate start out false
    builder->emitIndent();
    builder->append("compiler_meta__->drop = false;");
    builder->newline();
    builder->emitIndent();
    builder->append("compiler_meta__->recirculate = false;");
    builder->newline();
    builder->emitIndent();
    builder->append("compiler_meta__->egress_port = 0;");
    builder->newline();

    // workaround to make TC protocol-independent, DO NOT REMOVE
    builder->emitIndent();
    // replace ether_type only if a packet comes from XDP
    builder->appendFormat("if (!%v->recirculated) ", compilerGlobalMetadata);
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("compiler_meta__->mark = %u", packetMark);
    builder->endOfStatement(true);
    builder->emitIndent();
    // XDP2TC mode is assumed as META
    builder->append(
        "struct internal_metadata *md = "
        "(struct internal_metadata *)(unsigned long)skb->data_meta;\n"
        "        if ((void *) ((struct internal_metadata *) md + 1) <= "
        "(void *)(long)skb->data) {\n"
        "            __u16 *ether_type = (__u16 *) ((void *) (long)skb->data + 12);\n"
        "            if ((void *) ((__u16 *) ether_type + 1) > (void *) (long) skb->data_end) {\n"
        "                return TC_ACT_SHOT;\n"
        "            }\n"
        "            *ether_type = md->pkt_ether_type;\n"
        "        }\n");
    builder->blockEnd(true);
}

void TCIngressPipelinePNA::emitTrafficManager(EBPF::CodeBuilder *builder) {
    builder->emitIndent();
    builder->appendFormat("if (!%v->drop && %v->recirculate) ", compilerGlobalMetadata,
                          compilerGlobalMetadata);
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("%v->recirculated = true;", compilerGlobalMetadata);
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("return TC_ACT_UNSPEC;");
    builder->newline();
    builder->blockEnd(true);
    builder->emitIndent();
    builder->appendFormat("if (!%v->drop && %v->egress_port == 0)", compilerGlobalMetadata,
                          compilerGlobalMetadata);
    builder->newline();
    builder->increaseIndent();
    builder->emitIndent();
    builder->appendLine("return TC_ACT_OK;");
    builder->decreaseIndent();

    cstring eg_port = absl::StrFormat("%v->egress_port", compilerGlobalMetadata);
    cstring cos =
        absl::StrFormat("%v.class_of_service", control->outputStandardMetadata->name.name);
    builder->target->emitTraceMessage(
        builder, "IngressTM: Sending packet out of port %d with priority %d", 2, eg_port, cos);
    builder->emitIndent();
    builder->appendFormat("return bpf_redirect(%v->egress_port, 0)", compilerGlobalMetadata);
    builder->endOfStatement(true);
}

void TCIngressPipelinePNA::emitLocalVariables(EBPF::CodeBuilder *builder) {
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("unsigned %s_save = 0;", offsetVar.c_str());
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("%s %s = %s;", errorEnum.c_str(), errorVar.c_str(),
                          P4::P4CoreLibrary::instance().noError.str());
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("void* %s = %s;", packetStartVar.c_str(),
                          builder->target->dataOffset(model.CPacketName.toString()).c_str());
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("u8* %s = %s;", headerStartVar.c_str(), packetStartVar.c_str());
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("void* %s = %s;", packetEndVar.c_str(),
                          builder->target->dataEnd(model.CPacketName.toString()).c_str());
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("u32 %s = 0;", zeroKey.c_str());
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("u32 %s = 1;", oneKey.c_str());
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("unsigned char %s;", byteVar.c_str());
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("u32 %s = ", lengthVar.c_str());
    emitPacketLength(builder);
    builder->endOfStatement(true);

    if (shouldEmitTimestamp()) {
        builder->emitIndent();
        builder->appendFormat("u64 %s = ", timestampVar.c_str());
        emitTimestamp(builder);
        builder->endOfStatement(true);
    }
}

// =====================EBPFPnaParser=============================
EBPFPnaParser::EBPFPnaParser(const EBPF::EBPFProgram *program, const IR::ParserBlock *block,
                             const P4::TypeMap *typeMap)
    : EBPFPsaParser(program, block, typeMap) {
    visitor = new PnaStateTranslationVisitor(program->refMap, program->typeMap, this);
}

void EBPFPnaParser::emit(EBPF::CodeBuilder *builder) {
    builder->emitIndent();
    builder->blockStart();
    EBPF::EBPFParser::emit(builder);
    builder->blockEnd(true);
}

void EBPFPnaParser::emitRejectState(EBPF::CodeBuilder *builder) {
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
    builder->appendFormat("compiler_meta__->parser_error = %s", program->errorVar.c_str());
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("goto %s", IR::ParserState::accept.c_str());
    builder->endOfStatement(true);
}

void EBPFPnaParser::emitDeclaration(EBPF::CodeBuilder *builder, const IR::Declaration *decl) {
    if (auto di = decl->to<IR::Declaration_Instance>()) {
        cstring name = di->name.name;
        if (EBPFObject::getTypeName(di) == "InternetChecksum") {
            auto instance = new EBPFInternetChecksumPNA(program, di, name);
            checksums.emplace(name, instance);
            instance->emitVariables(builder);
            return;
        } else if (EBPFObject::getTypeName(di) == "Checksum") {
            auto instance = new EBPFCRCChecksumPNA(program, di, name);
            checksums.emplace(name, instance);
            instance->emitVariables(builder);
        }
    }

    EBPFParser::emitDeclaration(builder, decl);
}

//  This code is similar to compileExtractField function in PsaStateTranslationVisitor.
//  Handled TC "macaddr" annotation.
void PnaStateTranslationVisitor::compileExtractField(const IR::Expression *expr,
                                                     const IR::StructField *field,
                                                     unsigned hdrOffsetBits, EBPF::EBPFType *type) {
    unsigned alignment = hdrOffsetBits % 8;
    auto width = type->to<EBPF::IHasWidth>();
    if (width == nullptr) return;
    unsigned widthToExtract = width->widthInBits();
    auto program = state->parser->program;
    cstring msgStr;
    cstring fieldName = field->name.name;

    bool noEndiannessConversion = false;
    if (const auto *anno = field->getAnnotation(ParseTCAnnotations::tcType)) {
        cstring value = anno->getExpr(0)->checkedTo<IR::StringLiteral>()->value;
        if (value == "macaddr" || value == "ipv4" || value == "ipv6") {
            noEndiannessConversion = true;
        } else if (value == "be16" || value == "be32" || value == "be64") {
            std::string sInt = value.substr(2).c_str();
            unsigned int width = std::stoi(sInt);
            if (widthToExtract != width) {
                ::P4::error("Width of the field doesn't match the annotation width. '%1%'", field);
            }
            noEndiannessConversion = true;
        }
    }
    auto tcTarget = dynamic_cast<const EBPF::P4TCTarget *>(builder->target);
    bool isPrimitive = tcTarget->isPrimitiveByteAligned(widthToExtract);

    msgStr = absl::StrFormat("Parser: extracting field %v", fieldName);
    builder->target->emitTraceMessage(builder, msgStr.c_str());

    if (widthToExtract <= 64) {
        unsigned lastBitIndex = widthToExtract + alignment - 1;
        unsigned lastWordIndex = lastBitIndex / 8;
        unsigned wordsToRead = lastWordIndex + 1;
        unsigned loadSize;

        const char *helper = nullptr;
        if (wordsToRead <= 1) {
            helper = "load_byte";
            loadSize = 8;
        } else if (wordsToRead <= 2) {
            helper = "load_half";
            loadSize = 16;
        } else if (wordsToRead <= 4) {
            helper = "load_word";
            loadSize = 32;
        } else {
            if (wordsToRead > 64) BUG("Unexpected width %d", widthToExtract);
            helper = "load_dword";
            loadSize = 64;
        }

        unsigned shift = loadSize - alignment - widthToExtract;
        builder->emitIndent();
        if (noEndiannessConversion) {
            builder->appendFormat("__builtin_memcpy(&");
            visit(expr);
            builder->appendFormat(".%v, %v + BYTES(%v), %d)", fieldName, program->packetStartVar,
                                  program->offsetVar, widthToExtract / 8);
        } else {
            if (!isPrimitive) {
                cstring storePrimitive =
                    widthToExtract < 32 ? "storePrimitive32"_cs : "storePrimitive64"_cs;
                builder->appendFormat("%v((u8 *)&", storePrimitive);
                visit(expr);
                builder->appendFormat(".%v, %d, (", fieldName, widthToExtract);
                type->emit(builder);
                builder->appendFormat(")((%s(%v, BYTES(%v))", helper, program->packetStartVar,
                                      program->offsetVar);
                if (shift != 0) builder->appendFormat(" >> %d", shift);
                builder->append(")");
                if (widthToExtract != loadSize) {
                    builder->append(" & EBPF_MASK(");
                    type->emit(builder);
                    builder->appendFormat(", %d)", widthToExtract);
                }
                builder->append("))");
            } else {
                visit(expr);
                builder->appendFormat(".%v = (", fieldName);
                type->emit(builder);
                builder->appendFormat(")((%s(%v, BYTES(%v))", helper, program->packetStartVar,
                                      program->offsetVar);
                if (shift != 0) builder->appendFormat(" >> %d", shift);
                builder->append(")");
                if (widthToExtract != loadSize) {
                    builder->append(" & EBPF_MASK(");
                    type->emit(builder);
                    builder->appendFormat(", %d)", widthToExtract);
                }
                builder->append(")");
            }
        }
        builder->endOfStatement(true);
    } else {
        if (program->options.arch == "psa" && widthToExtract % 8 != 0) {
            // To explain the problem in error lets assume that we have bit<68> field with value:
            //   0x11223344556677889
            //                     ^ this digit will be parsed into a half of byte
            // Such fields are parsed into a table of bytes in network byte order, so possible
            // values in dataplane are (note the position of additional '0' at the end):
            //   0x112233445566778809
            //   0x112233445566778890
            // To correctly insert that padding, the length of field must be known, but tools like
            // nikss-ctl (and the nikss library) don't consume P4info.txt to have such knowledge.
            // There is also a bug in (de)parser causing such fields to be deparsed incorrectly.
            ::P4::error(
                ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "%1%: fields wider than 64 bits must have a size multiple of 8 bits (1 byte) "
                "due to ambiguous padding in the LSB byte when the condition is not met",
                field);
        }

        // wide values; read all bytes one by one.
        unsigned shift;
        if (alignment == 0)
            shift = 0;
        else
            shift = 8 - alignment;

        const char *helper;
        if (shift == 0)
            helper = "load_byte";
        else
            helper = "load_half";
        auto bt = EBPF::EBPFTypeFactory::instance->create(IR::Type_Bits::get(8));
        unsigned bytes = ROUNDUP(widthToExtract, 8);
        for (unsigned i = 0; i < bytes; i++) {
            builder->emitIndent();
            visit(expr);
            builder->appendFormat(".%s[%d] = (", fieldName.c_str(), i);
            bt->emit(builder);
            builder->appendFormat(")((%s(%s, BYTES(%s) + %d) >> %d)", helper,
                                  program->packetStartVar.c_str(), program->offsetVar.c_str(), i,
                                  shift);

            if ((i == bytes - 1) && (widthToExtract % 8 != 0)) {
                builder->append(" & EBPF_MASK(");
                bt->emit(builder);
                builder->appendFormat(", %d)", widthToExtract % 8);
            }

            builder->append(")");
            builder->endOfStatement(true);
        }
    }

    builder->emitIndent();
    builder->appendFormat("%s += %d", program->offsetVar.c_str(), widthToExtract);
    builder->endOfStatement(true);

    // eBPF can pass 64 bits of data as one argument passed in 64 bit register,
    // so value of the field is printed only when it fits into that register
    if (widthToExtract <= 64) {
        cstring exprStr = expr->toString();
        if (auto member = expr->to<IR::Member>()) {
            if (auto pathExpr = member->expr->to<IR::PathExpression>()) {
                if (isPointerVariable(pathExpr->path->name.name)) {
                    exprStr = exprStr.replace(".", "->");
                }
            }
        }
        cstring tmp = absl::StrFormat("(unsigned long long) %v.%v", exprStr, fieldName);

        msgStr =
            absl::StrFormat("Parser: extracted %v=0x%%llx (%u bits)", fieldName, widthToExtract);
        builder->target->emitTraceMessage(builder, msgStr.c_str(), 1, tmp.c_str());
    } else {
        msgStr = absl::StrFormat("Parser: extracted %v (%u bits)", fieldName, widthToExtract);
        builder->target->emitTraceMessage(builder, msgStr.c_str());
    }

    builder->newline();
}

bool PnaStateTranslationVisitor::preorder(const IR::Member *m) {
    if ((m->expr != nullptr) && (m->expr->type != nullptr)) {
        if (auto st = m->expr->type->to<IR::Type_Struct>()) {
            if (st->name == "pna_main_parser_input_metadata_t") {
                if (m->member.name == "input_port") {
                    builder->append("skb->ifindex");
                    return false;
                } else {
                    ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                                "%1%: this metadata field is not supported", m);
                }
            }
        }
    }

    return EBPF::PsaStateTranslationVisitor::preorder(m);
}

bool PnaStateTranslationVisitor::preorder(const IR::AssignmentStatement *statement) {
    auto ltype = typeMap->getType(statement->left);

    if (auto mce = statement->right->to<IR::MethodCallExpression>()) {
        auto mi = P4::MethodInstance::resolve(mce, state->parser->program->refMap,
                                              state->parser->program->typeMap);
        auto extMethod = mi->to<P4::ExternMethod>();
        if (extMethod == nullptr) BUG("Unhandled method %1%", mce);

        auto decl = extMethod->object;
        if (decl == state->parser->packet) {
            if (extMethod->method->name.name == p4lib.packetIn.lookahead.name) {
                compileLookahead(statement->left);
                return false;
            } else if (extMethod->method->name.name == p4lib.packetIn.length.name) {
                builder->emitIndent();
                emitTCAssignmentEndianessConversion(ltype, statement->left, statement->right,
                                                    nullptr);
                builder->endOfStatement();
                return false;
            }
        }
    }

    builder->emitIndent();
    emitTCAssignmentEndianessConversion(ltype, statement->left, statement->right, nullptr);
    builder->endOfStatement();
    return false;
}

void PnaStateTranslationVisitor::compileLookahead(const IR::Expression *destination) {
    cstring msgStr = absl::StrFormat("Parser: lookahead for %v %v",
                                     state->parser->typeMap->getType(destination), destination);
    builder->target->emitTraceMessage(builder, msgStr.c_str());

    builder->emitIndent();
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("u8* %s_save = %s", state->parser->program->headerStartVar.c_str(),
                          state->parser->program->headerStartVar.c_str());
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("unsigned %s_save = %s", state->parser->program->offsetVar.c_str(),
                          state->parser->program->offsetVar.c_str());
    builder->endOfStatement(true);
    compileExtract(destination);
    builder->emitIndent();
    builder->appendFormat("%s = %s_save", state->parser->program->headerStartVar.c_str(),
                          state->parser->program->headerStartVar.c_str());
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("%s = %s_save", state->parser->program->offsetVar.c_str(),
                          state->parser->program->offsetVar.c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);
}

bool PnaStateTranslationVisitor::preorder(const IR::SelectCase *selectCase) {
    unsigned width = EBPF::EBPFInitializerUtils::ebpfTypeWidth(typeMap, selectCase->keyset);
    bool scalar = EBPF::EBPFScalarType::generatesScalar(width);
    auto ebpfType = EBPF::EBPFTypeFactory::instance->create(selectType);
    unsigned selectWidth = 0;
    if (ebpfType->is<EBPF::EBPFScalarType>()) {
        selectWidth = ebpfType->to<EBPF::EBPFScalarType>()->implementationWidthInBits();
    }
    auto tcTarget = dynamic_cast<const EBPF::P4TCTarget *>(builder->target);
    bool isPrimitive = tcTarget->isPrimitiveByteAligned(selectWidth);

    builder->emitIndent();
    if (auto pe = selectCase->keyset->to<IR::PathExpression>()) {
        builder->append("if (");
        cstring pvsName = pe->path->name.name;
        auto pvs = state->parser->getValueSet(pvsName);
        if (pvs) pvs->emitLookup(builder);
        builder->append(" != NULL)");
    } else if (auto mask = selectCase->keyset->to<IR::Mask>()) {
        if (scalar) {
            if (!isPrimitive) {
                cstring getPrimitive = selectWidth < 32 ? "getPrimitive32"_cs : "getPrimitive64"_cs;
                builder->appendFormat("if ((%v((u8 *)%v, %d)", getPrimitive, selectValue,
                                      selectWidth);
            } else {
                builder->appendFormat("if ((%v", selectValue);
            }
            builder->append(" & ");
            visit(mask->right);
            builder->append(") == (");
            visit(mask->left);
            builder->append(" & ");
            visit(mask->right);
            builder->append("))");
        } else {
            unsigned bytes = ROUNDUP(width, 8);
            bool first = true;
            cstring hex = EBPF::EBPFInitializerUtils::genHexStr(
                mask->right->to<IR::Constant>()->value, width, mask->right);
            cstring value = EBPF::EBPFInitializerUtils::genHexStr(
                mask->left->to<IR::Constant>()->value, width, mask->left);
            builder->append("if (");
            for (unsigned i = 0; i < bytes; ++i) {
                if (!first) {
                    builder->append(" && ");
                }
                builder->appendFormat("((%v[%u] & 0x%v)", selectValue, i, hex.substr(2 * i, 2));
                builder->append(" == ");
                builder->appendFormat("(%v & %v))", value.substr(2 * i, 2), hex.substr(2 * i, 2));
                first = false;
            }
            builder->append(") ");
        }
    } else {
        if (!isPrimitive) {
            cstring getPrimitive = selectWidth < 32 ? "getPrimitive32"_cs : "getPrimitive64"_cs;
            builder->appendFormat("if (%v((u8 *)%v, %d)", getPrimitive, selectValue, selectWidth);
        } else {
            builder->appendFormat("if (%v", selectValue);
        }
        builder->append(" == ");
        visit(selectCase->keyset);
        builder->append(")");
    }
    builder->append("goto ");
    visit(selectCase->state);
    builder->endOfStatement(true);
    return false;
}

bool PnaStateTranslationVisitor::preorder(const IR::SelectExpression *expression) {
    BUG_CHECK(expression->select->components.size() == 1, "%1%: tuple not eliminated in select",
              expression->select);
    selectValue = state->parser->program->refMap->newName("select");
    selectType = state->parser->program->typeMap->getType(expression->select, true);
    if (auto list = selectType->to<IR::Type_List>()) {
        BUG_CHECK(list->components.size() == 1, "%1% list type with more than 1 element", list);
        selectType = list->components.at(0);
    }
    auto etype = EBPF::EBPFTypeFactory::instance->create(selectType);
    builder->emitIndent();
    etype->declare(builder, selectValue, false);
    builder->endOfStatement(true);

    builder->emitIndent();
    emitTCAssignmentEndianessConversion(selectType, nullptr, expression->select->components.at(0),
                                        selectValue);
    builder->endOfStatement();
    builder->newline();

    // Init value_sets
    for (auto e : expression->selectCases) {
        if (e->keyset->is<IR::PathExpression>()) {
            cstring pvsName = e->keyset->to<IR::PathExpression>()->path->name.name;
            cstring pvsKeyVarName = state->parser->program->refMap->newName(pvsName + "_key");
            auto pvs = state->parser->getValueSet(pvsName);
            if (pvs != nullptr)
                pvs->emitKeyInitializer(builder, expression, pvsKeyVarName);
            else
                ::P4::error(ErrorType::ERR_UNKNOWN, "%1%: expected a value_set instance",
                            e->keyset);
        }
    }

    for (auto e : expression->selectCases) visit(e);

    builder->emitIndent();
    builder->appendFormat("else goto %s;", IR::ParserState::reject.c_str());
    builder->newline();
    return false;
}

// =====================EBPFTablePNA=============================
void EBPFTablePNA::emitKeyType(EBPF::CodeBuilder *builder) {
    builder->emitIndent();
    builder->appendFormat("struct __attribute__((__packed__)) %s ", keyTypeName.c_str());
    builder->blockStart();

    EBPF::CodeGenInspector commentGen(program->refMap, program->typeMap);
    commentGen.setBuilder(builder);

    unsigned int structAlignment = 8;  // by default

    builder->emitIndent();
    builder->appendLine("u32 keysz;");
    builder->emitIndent();
    builder->appendLine("u32 maskid;");

    if (keyGenerator != nullptr) {
        for (auto c : keyGenerator->keyElements) {
            auto mtdecl = program->refMap->getDeclaration(c->matchType->path, true);
            auto matchType = mtdecl->getNode()->checkedTo<IR::Declaration_ID>();

            if (!isMatchTypeSupported(matchType)) {
                ::P4::error(ErrorType::ERR_UNSUPPORTED, "Match of type %1% not supported",
                            c->matchType);
            }

            auto ebpfType = ::P4::get(keyTypes, c);
            cstring fieldName = ::P4::get(keyFieldNames, c);

            if (ebpfType->is<EBPF::EBPFScalarType>() &&
                ebpfType->as<EBPF::EBPFScalarType>().alignment() > structAlignment) {
                structAlignment = 8;
            }

            builder->emitIndent();
            ebpfType->declare(builder, fieldName, false);

            builder->append("; /* ");
            c->expression->apply(commentGen);
            builder->append(" */");
            builder->newline();
        }
    }

    builder->blockEnd(false);
    builder->appendFormat(" __attribute__((aligned(%d)))", structAlignment);
    builder->endOfStatement(true);

    if (isTernaryTable()) {
        // generate mask key
        builder->emitIndent();
        builder->appendFormat("#define MAX_%v_MASKS %u", keyTypeName.toUpper(),
                              program->options.maxTernaryMasks);
        builder->newline();

        builder->emitIndent();
        builder->appendFormat("struct %s_mask ", keyTypeName.c_str());
        builder->blockStart();

        builder->emitIndent();
        builder->appendFormat("__u8 mask[sizeof(struct %s)];", keyTypeName.c_str());
        builder->newline();

        builder->blockEnd(false);
        builder->appendFormat(" __attribute__((aligned(%d)))", structAlignment);
        builder->endOfStatement(true);
    }
}

void EBPFTablePNA::emitValueType(EBPF::CodeBuilder *builder) {
    emitValueActionIDNames(builder);

    // a type-safe union: a struct with a tag and an union
    builder->emitIndent();
    builder->appendFormat("struct __attribute__((__packed__)) %s ", valueTypeName.c_str());
    builder->blockStart();

    emitValueStructStructure(builder);

    builder->blockEnd(false);
    builder->endOfStatement(true);
}

void EBPFTablePNA::emitValueStructStructure(EBPF::CodeBuilder *builder) {
    builder->emitIndent();
    builder->append("unsigned int action;");
    builder->newline();

    builder->emitIndent();
    builder->append("u32 hit:1,");
    builder->newline();

    builder->emitIndent();
    builder->append("is_default_miss_act:1,");
    builder->newline();

    builder->emitIndent();
    builder->append("is_default_hit_act:1;");
    builder->newline();

    builder->emitIndent();
    builder->append("union ");
    builder->blockStart();

    // Declare NoAction data structure at the beginning as it has reserved id 0
    builder->emitIndent();
    builder->appendLine("struct {");
    builder->emitIndent();
    builder->append("} _NoAction");
    builder->endOfStatement(true);

    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->checkedTo<IR::P4Action>();
        if (action->name.originalName == P4::P4CoreLibrary::instance().noAction.name) continue;
        cstring name = EBPF::EBPFObject::externalName(action);
        emitActionArguments(builder, action, name);
    }

    builder->blockEnd(false);
    builder->spc();
    builder->appendLine("u;");
}

cstring EBPFTablePNA::p4ActionToActionIDName(const IR::P4Action *action) const {
    cstring actionName = EBPFObject::externalName(action);
    cstring tableInstance = dataMapName;
    return absl::StrFormat("%v_ACT_%v", tableInstance.toUpper(), actionName.toUpper());
}

void EBPFTablePNA::emitActionArguments(EBPF::CodeBuilder *builder, const IR::P4Action *action,
                                       cstring name) {
    builder->emitIndent();
    if (action->parameters->empty())
        builder->append("struct ");
    else
        builder->append("struct __attribute__((__packed__)) ");
    builder->blockStart();

    for (auto p : *action->parameters->getEnumerator()) {
        builder->emitIndent();
        auto type = EBPF::EBPFTypeFactory::instance->create(p->type);
        type->declare(builder, p->externalName(), false);
        builder->endOfStatement(true);
    }

    builder->blockEnd(false);
    builder->spc();
    builder->append(name);
    builder->endOfStatement(true);
}

void EBPFTablePNA::emitAction(EBPF::CodeBuilder *builder, cstring valueName,
                              cstring actionRunVariable) {
    builder->blockStart();
    builder->emitIndent();
    builder->appendLine("/* run action */");
    builder->emitIndent();
    builder->appendFormat("switch (%s->action) ", valueName.c_str());
    builder->blockStart();
    bool noActionGenerated = false;

    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->checkedTo<IR::P4Action>();
        if (action->name.originalName == P4::P4CoreLibrary::instance().noAction.name)
            noActionGenerated = true;
        cstring name = EBPF::EBPFObject::externalName(action), msgStr, convStr;
        builder->emitIndent();
        cstring actionName = p4ActionToActionIDName(action);
        builder->appendFormat("case %v: ", actionName);
        builder->newline();
        builder->increaseIndent();

        msgStr = absl::StrFormat("Control: executing action %v", name);
        builder->target->emitTraceMessage(builder, msgStr.c_str());
        for (auto param : *(action->parameters)) {
            auto etype = EBPF::EBPFTypeFactory::instance->create(param->type);
            unsigned width = etype->as<EBPF::IHasWidth>().widthInBits();

            if (width <= 64) {
                convStr =
                    absl::StrFormat("(unsigned long long) (%v->u.%v.%v)", valueName, name, param);
                msgStr = absl::StrFormat("Control: param %v=0x%%llx (%d bits)", param, width);
                builder->target->emitTraceMessage(builder, msgStr.c_str(), 1, convStr.c_str());
            } else {
                msgStr = absl::StrFormat("Control: param %v (%d bits)", param, width);
                builder->target->emitTraceMessage(builder, msgStr.c_str());
            }
        }
        bool generateDefaultMissCode = false;
        bool generateDefaultHitCode = false;
        for (auto tbl : tcIR->tcPipeline->tableDefs) {
            if (tbl->getTableName() == table->container->name.originalName) {
                if (tbl->isTcMayOverrideMiss) {
                    cstring defaultActionName = tbl->defaultMissAction->getActionName();
                    defaultActionName = defaultActionName.substr(
                        defaultActionName.find('/') - defaultActionName.begin() + 1,
                        defaultActionName.size());
                    if (defaultActionName == action->name.originalName)
                        generateDefaultMissCode = true;
                }
                if (tbl->isTcMayOverrideHit) {
                    cstring defaultActionName = tbl->defaultHitAction->getActionName();
                    defaultActionName = defaultActionName.substr(
                        defaultActionName.find('/') - defaultActionName.begin() + 1,
                        defaultActionName.size());
                    if (defaultActionName == action->name.originalName)
                        generateDefaultHitCode = true;
                }
            }
        }
        if (generateDefaultMissCode || generateDefaultHitCode) {
            builder->emitIndent();
            builder->appendFormat("{");
            builder->newline();
            builder->increaseIndent();

            builder->emitIndent();
            if (generateDefaultMissCode)
                builder->appendFormat("if (%s->is_default_miss_act) ", valueName.c_str());
            else if (generateDefaultHitCode)
                builder->appendFormat("if (%s->is_default_hit_act) ", valueName.c_str());
            builder->newline();

            builder->emitIndent();
            auto defaultVisitor = createActionTranslationVisitor(valueName, program, action, true);
            defaultVisitor->setBuilder(builder);
            defaultVisitor->copySubstitutions(codeGen);
            defaultVisitor->copyPointerVariables(codeGen);
            action->apply(*defaultVisitor);
            builder->newline();

            builder->emitIndent();
            builder->appendFormat("else");
            builder->newline();

            builder->emitIndent();
            auto visitor = createActionTranslationVisitor(valueName, program, action, false);
            visitor->setBuilder(builder);
            visitor->copySubstitutions(codeGen);
            visitor->copyPointerVariables(codeGen);
            action->apply(*visitor);
            builder->newline();

            builder->decreaseIndent();
            builder->emitIndent();
            builder->appendFormat("}");
            builder->newline();
        } else {
            builder->emitIndent();
            auto visitor = createActionTranslationVisitor(valueName, program, action, false);
            visitor->setBuilder(builder);
            visitor->copySubstitutions(codeGen);
            visitor->copyPointerVariables(codeGen);
            action->apply(*visitor);
            builder->newline();
        }

        builder->emitIndent();
        builder->appendLine("break;");
        builder->decreaseIndent();
    }
    if (!noActionGenerated) {
        cstring noActionName = P4::P4CoreLibrary::instance().noAction.name;
        cstring tableInstance = dataMapName;
        cstring actionName =
            absl::StrFormat("%v_ACT_%v", tableInstance.toUpper(), noActionName.toUpper());
        builder->emitIndent();
        builder->appendFormat("case %v: ", actionName);
        builder->newline();
        builder->increaseIndent();
        builder->emitIndent();
        builder->blockStart();
        builder->blockEnd(true);
        builder->emitIndent();
        builder->appendLine("break;");
        builder->decreaseIndent();
    }

    builder->blockEnd(true);

    if (!actionRunVariable.isNullOrEmpty()) {
        builder->emitIndent();
        builder->appendFormat("%s = %s->action", actionRunVariable.c_str(), valueName.c_str());
        builder->endOfStatement(true);
    }

    builder->blockEnd(false);
    builder->appendFormat(" else ");
    builder->blockStart();
    builder->blockEnd(true);
}

void EBPFTablePNA::emitInitializer(EBPF::CodeBuilder *builder) {
    // Do not emit initializer when table implementation is provided, because it is not supported.
    // Error for such case is printed when adding implementation to a table.
    if (implementation == nullptr) {
        emitDefaultActionStruct(builder);
        this->emitConstEntriesInitializer(builder);
    }
}

void EBPFTablePNA::emitDefaultActionStruct(EBPF::CodeBuilder *builder) {
    const IR::P4Table *t = table->container;
    const IR::Expression *defaultAction = t->getDefaultAction();
    auto actionName = getActionNameExpression(defaultAction);
    CHECK_NULL(actionName);
    if (actionName->path->name.originalName != P4::P4CoreLibrary::instance().noAction.name) {
        auto value = program->refMap->newName("value");
        emitTableValue(builder, defaultAction, value);
    }
}

void EBPFTablePNA::emitValueActionIDNames(EBPF::CodeBuilder *builder) {
    // create type definition for action
    builder->emitIndent();
    bool noActionGenerated = false;
    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->checkedTo<IR::P4Action>();
        if (action->name.originalName == P4::P4CoreLibrary::instance().noAction.name)
            noActionGenerated = true;
        unsigned int action_idx = tcIR->getActionId(tcIR->externalName(action));
        builder->emitIndent();
        builder->appendFormat("#define %v %d", p4ActionToActionIDName(action), action_idx);
        builder->newline();
    }
    if (!noActionGenerated) {
        cstring noActionName = P4::P4CoreLibrary::instance().noAction.name;
        cstring tableInstance = dataMapName;
        cstring actionName =
            absl::StrFormat("%v_ACT_%v", tableInstance.toUpper(), noActionName.toUpper());
        unsigned int action_idx = tcIR->getActionId(noActionName);
        builder->emitIndent();
        builder->appendFormat("#define %v %d", actionName, action_idx);
        builder->newline();
    }
    builder->emitIndent();
}

void EBPFTablePNA::initDirectCounters() {
    EBPFTablePNADirectCounterPropertyVisitor visitor(this);
    visitor.visitTableProperty();
}

void EBPFTablePNA::initDirectMeters() {
    EBPFTablePNADirectMeterPropertyVisitor visitor(this);
    visitor.visitTableProperty();
}

// =====================IngressDeparserPNA=============================
bool IngressDeparserPNA::build() {
    auto pl = controlBlock->container->type->applyParams;

    if (pl->size() != 4) {
        ::P4::error(ErrorType::ERR_EXPECTED, "Expected deparser to have exactly 4 parameters");
        return false;
    }

    auto it = pl->parameters.begin();
    packet_out = *it;
    headers = *(it + 1);
    user_metadata = *(it + 2);

    auto ht = program->typeMap->getType(headers);
    if (ht == nullptr) {
        return false;
    }
    headerType = EBPF::EBPFTypeFactory::instance->create(ht);

    return true;
}

void IngressDeparserPNA::emitPreDeparser(EBPF::CodeBuilder *builder) {
    builder->emitIndent();
    CHECK_NULL(program);
    auto pipeline = program->checkedTo<EBPF::EBPFPipeline>();
    builder->appendFormat("if (%v->drop) ", pipeline->compilerGlobalMetadata);
    builder->blockStart();
    builder->target->emitTraceMessage(builder, "PreDeparser: dropping packet..");
    builder->emitIndent();
    builder->appendFormat("return %s;\n", builder->target->abortReturnCode().c_str());
    builder->blockEnd(true);
}

void IngressDeparserPNA::emit(EBPF::CodeBuilder *builder) {
    codeGen->setBuilder(builder);
    emitExternDefinition(builder);
    for (auto a : controlBlock->container->controlLocals) {
        if (a->is<IR::Declaration_Variable>()) {
            auto vd = a->to<IR::Declaration_Variable>();
            if (vd->type->toString() == headers->type->toString() ||
                vd->type->toString() == user_metadata->type->toString()) {
                codeGen->isPointerVariable(a->name.name);
                codeGen->useAsPointerVariable(vd->name);
            }
        }
        emitDeclaration(builder, a);
    }

    emitDeparserExternCalls(builder);
    builder->newline();

    emitPreDeparser(builder);

    builder->emitIndent();
    builder->appendFormat("int %s = 0", this->outerHdrLengthVar.c_str());
    builder->endOfStatement(true);

    auto prepareBufferTranslator = new EBPF::DeparserPrepareBufferTranslator(this);
    prepareBufferTranslator->setBuilder(builder);
    prepareBufferTranslator->copyPointerVariables(codeGen);
    prepareBufferTranslator->substitute(this->headers, this->parserHeaders);
    controlBlock->container->body->apply(*prepareBufferTranslator);

    builder->newline();
    builder->emitIndent();
    builder->appendFormat("__u16 saved_proto = 0");
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("bool have_saved_proto = false");
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendLine("// bpf_skb_adjust_room works only when protocol is IPv4 or IPv6");
    builder->emitIndent();
    builder->appendLine("// 0x0800 = IPv4, 0x86dd = IPv6");
    builder->emitIndent();
    builder->append(
        "if ((skb->protocol != bpf_htons(0x0800)) && (skb->protocol != bpf_htons(0x86dd))) ");
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("saved_proto = skb->protocol");
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("have_saved_proto = true");
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("bpf_p4tc_skb_set_protocol(skb, &sa->set, bpf_htons(0x0800))");
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("bpf_p4tc_skb_meta_set(skb, &sa->set, sizeof(sa->set))");
    builder->endOfStatement(true);
    builder->blockEnd(true);
    builder->emitIndent();
    builder->endOfStatement(true);

    emitBufferAdjusts(builder);

    builder->newline();
    builder->emitIndent();
    builder->append("if (have_saved_proto) ");
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("bpf_p4tc_skb_set_protocol(skb, &sa->set, saved_proto)");
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("bpf_p4tc_skb_meta_set(skb, &sa->set, sizeof(sa->set))");
    builder->endOfStatement(true);
    builder->blockEnd(true);
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("%v = %v;", program->packetStartVar,
                          builder->target->dataOffset(program->model.CPacketName.toString()));
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("%v = %v;", program->packetEndVar,
                          builder->target->dataEnd(program->model.CPacketName.toString()));
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("%s = 0", program->offsetVar.c_str());
    builder->endOfStatement(true);

    // emit headers
    auto hdrEmitTranslator = new DeparserHdrEmitTranslatorPNA(this);
    hdrEmitTranslator->setBuilder(builder);
    hdrEmitTranslator->copyPointerVariables(codeGen);
    hdrEmitTranslator->substitute(this->headers, this->parserHeaders);
    controlBlock->container->body->apply(*hdrEmitTranslator);

    builder->newline();
}

void IngressDeparserPNA::emitDeclaration(EBPF::CodeBuilder *builder, const IR::Declaration *decl) {
    if (auto di = decl->to<IR::Declaration_Instance>()) {
        cstring name = di->name.name;

        if (EBPF::EBPFObject::getTypeName(di) == "InternetChecksum") {
            auto instance = new EBPFInternetChecksumPNA(program, di, name);
            checksums.emplace(name, instance);
            instance->emitVariables(builder);
            return;
        } else if (EBPF::EBPFObject::getSpecializedTypeName(di) == "Checksum") {
            auto instance = new EBPFCRCChecksumPNA(program, di, name);
            checksums.emplace(name, instance);
            instance->emitVariables(builder);
            return;
        }
    }

    EBPFDeparser::emitDeclaration(builder, decl);
}

static void gen_skb_call(EBPF::CodeBuilder *b, const P4::ExternMethod *m,
                         EBPF::CodeGenInspector *xlat) {
    b->appendFormat("/* SKB metadata: call %s */", m->method->name.name.c_str());
    if (m->method->name.name == "get") {
        if (!m->expr->arguments->empty()) {
            ::P4::error("%1%: takes no arguments", m->method->name.name);
        }
        b->appendFormat("bpf_p4tc_skb_meta_get(skb,&sa->get,sizeof(sa->get))");
    } else if (m->method->name.name == "set") {
        if (!m->expr->arguments->empty()) {
            ::P4::error("%1%: takes no arguments", m->method->name.name);
        }
        b->appendFormat("bpf_p4tc_skb_meta_set(skb,&sa->set,sizeof(sa->set))");
    }
#define GETSET(n)                                                                  \
    else if (m->method->name.name == ("get_" #n)) {                                \
        if (!m->expr->arguments->empty()) {                                        \
            P4::error("%1%: takes no arguments", m->method->name.name);            \
        }                                                                          \
        b->append("bpf_p4tc_skb_get_" #n "(skb,&sa->get)");                        \
    }                                                                              \
    else if (m->method->name.name == ("set_" #n)) {                                \
        if (m->expr->arguments->size() != 1) {                                     \
            P4::error("%1%: requires exactly one argument", m->method->name.name); \
        }                                                                          \
        b->append("bpf_p4tc_skb_set_" #n "(skb,&sa->set,");                        \
        xlat->visit(m->expr->arguments->at(0)->expression);                        \
        b->append(")");                                                            \
    }
    GETSET(tstamp)
    GETSET(mark)
    GETSET(tc_classid)
    GETSET(tc_index)
    GETSET(queue_mapping)
    GETSET(protocol)
    GETSET(tc_at_ingress)
    GETSET(from_ingress)
#undef GETSET
    else {
        P4::error("Unsupported SKB metadata method call %1%", m->method->name.name);
    }
}

static bool is_skb_meta_func(const P4::cstring fname) {
    return (fname == "skb_get_meta") || (fname == "skb_set_tstamp") || (fname == "skb_set_mark") ||
           (fname == "skb_set_queue_mapping") || (fname == "skb_set_protocol") ||
           (fname == "skb_set_tc_classid") || (fname == "skb_set_tc_index") ||
           (fname == "skb_set_meta");
}

void DeparserBodyTranslatorPNA::processFunction(const P4::ExternFunction *function) {
    auto fname = function->expr->method->toString();

    if (is_skb_meta_func(fname)) {
        ::P4::error(ErrorType::ERR_UNEXPECTED,
                    "Unexpected call to extern function %s in the deparser", fname);
    }
}

bool DeparserBodyTranslatorPNA::preorder(const IR::AssignmentStatement *a) {
    auto ltype = typeMap->getType(a->left);

    builder->emitIndent();
    emitTCAssignmentEndianessConversion(ltype, a->left, a->right, nullptr);
    builder->endOfStatement();
    return false;
}

// =====================ConvertToEbpfPNA=============================
const PNAEbpfGenerator *ConvertToEbpfPNA::build(const IR::ToplevelBlock *tlb) {
    /*
     * TYPES
     */
    std::vector<EBPF::EBPFType *> ebpfTypes;
    for (auto d : tlb->getProgram()->objects) {
        if (d->is<IR::Type>() && !d->is<IR::IContainer>() && !d->is<IR::Type_Extern>() &&
            !d->is<IR::Type_Parser>() && !d->is<IR::Type_Control>() && !d->is<IR::Type_Typedef>() &&
            !d->is<IR::Type_Error>()) {
            if (d->srcInfo.isValid()) {
                auto sourceFile = d->srcInfo.getSourceFile();
                if (sourceFile.endsWith("/pna.p4")) {
                    // do not generate standard PNA types
                    continue;
                }
            }

            auto type = EBPF::EBPFTypeFactory::instance->create(d->to<IR::Type>());
            if (type == nullptr) continue;
            ebpfTypes.push_back(type);
        }
    }

    /*
     * PIPELINE
     */
    auto pipeline = tlb->getMain()->checkedTo<IR::PackageBlock>();
    auto pipelineParser = pipeline->getParameterValue("main_parser"_cs);
    BUG_CHECK(pipelineParser != nullptr, "No parser block found");
    auto pipelineControl = pipeline->getParameterValue("main_control"_cs);
    BUG_CHECK(pipelineControl != nullptr, "No control block found");
    auto pipelineDeparser = pipeline->getParameterValue("main_deparser"_cs);
    BUG_CHECK(pipelineDeparser != nullptr, "No deparser block found");

    auto xdp = new EBPF::XDPHelpProgram(options);

    auto pipeline_converter = new ConvertToEbpfPipelineTC(
        "tc-ingress"_cs, EBPF::TC_INGRESS, options, pipelineParser->checkedTo<IR::ParserBlock>(),
        pipelineControl->checkedTo<IR::ControlBlock>(),
        pipelineDeparser->checkedTo<IR::ControlBlock>(), refmap, typemap, tcIR, ebpfTypes);
    pipeline->apply(*pipeline_converter);
    tlb->getProgram()->apply(*pipeline_converter);
    auto tcIngress = pipeline_converter->getEbpfPipeline();

    return new PNAArchTC(options, ebpfTypes, xdp, tcIngress, tcIR);
}

const IR::Node *ConvertToEbpfPNA::preorder(IR::ToplevelBlock *tlb) {
    ebpf_program = build(tlb);
    ebpf_program->pipeline->program = tlb->getProgram();
    return tlb;
}

// =====================ConvertToEbpfPipelineTC=============================
bool ConvertToEbpfPipelineTC::preorder(const IR::PackageBlock *block) {
    (void)block;

    pipeline = new TCIngressPipelinePNA(name, options, refmap, typemap);
    pipeline->sectionName = "p4tc/main"_cs;
    auto parser_converter = new ConvertToEBPFParserPNA(pipeline, typemap);
    parserBlock->apply(*parser_converter);
    pipeline->parser = parser_converter->getEBPFParser();
    CHECK_NULL(pipeline->parser);

    auto control_converter =
        new ConvertToEBPFControlPNA(pipeline, pipeline->parser->headers, refmap, type, tcIR);
    controlBlock->apply(*control_converter);
    pipeline->control = control_converter->getEBPFControl();
    CHECK_NULL(pipeline->control);

    auto deparser_converter =
        new ConvertToEBPFDeparserPNA(pipeline, pipeline->parser->headers,
                                     pipeline->control->outputStandardMetadata, tcIR, ebpfTypes);
    deparserBlock->apply(*deparser_converter);
    pipeline->deparser = deparser_converter->getEBPFDeparser();
    CHECK_NULL(pipeline->deparser);

    return true;
}

// =====================ConvertToEBPFParserPNA=============================
bool ConvertToEBPFParserPNA::preorder(const IR::ParserBlock *prsr) {
    auto pl = prsr->container->type->applyParams;
    parser = new TC::EBPFPnaParser(program, prsr, typemap);
    unsigned numOfParams = 4;

    if (pl->size() != numOfParams) {
        ::P4::error(ErrorType::ERR_EXPECTED, "Expected parser to have exactly %1% parameters",
                    numOfParams);
        return false;
    }

    auto it = pl->parameters.begin();
    parser->packet = *it;
    ++it;
    parser->headers = *it;
    ++it;
    parser->user_metadata = *it;
    ++it;
    parser->inputMetadata = *it;
    ++it;

    for (auto state : prsr->container->states) {
        auto ps = new EBPF::EBPFParserState(state, parser);
        parser->states.push_back(ps);
    }

    auto ht = typemap->getType(parser->headers);
    if (ht == nullptr) return false;
    parser->headerType = EBPF::EBPFTypeFactory::instance->create(ht);

    parser->visitor->useAsPointerVariable(parser->user_metadata->name.name);
    parser->visitor->useAsPointerVariable(parser->headers->name.name);

    return true;
}

bool ConvertToEBPFParserPNA::preorder(const IR::P4ValueSet *pvs) {
    cstring extName = EBPF::EBPFObject::externalName(pvs);
    auto instance = new EBPF::EBPFValueSet(program, pvs, extName, parser->visitor);
    parser->valueSets.emplace(pvs->name.name, instance);

    return false;
}

void EBPFControlPNA::emit(EBPF::CodeBuilder *builder) {
    for (auto h : pna_hashes) h.second->emitVariables(builder);
    EBPF::EBPFControl::emit(builder);
}

// =====================ConvertToEBPFControlPNA=============================
bool ConvertToEBPFControlPNA::preorder(const IR::ControlBlock *ctrl) {
    control = new EBPFControlPNA(program, ctrl, parserHeaders);
    program->control = control;
    program->as<EBPF::EBPFPipeline>().control = control;
    control->hitVariable = refmap->newName("hit");
    auto pl = ctrl->container->type->applyParams;
    unsigned numOfParams = 4;
    if (pl->size() != numOfParams) {
        ::P4::error(ErrorType::ERR_EXPECTED, "Expected control to have exactly %1% parameters",
                    numOfParams);
        return false;
    }
    auto it = pl->parameters.begin();
    control->headers = *it;
    ++it;
    control->user_metadata = *it;
    ++it;
    control->inputStandardMetadata = *it;
    ++it;
    control->outputStandardMetadata = *it;

    control->touched_skb_metadata = false;

    auto codegen = new ControlBodyTranslatorPNA(control, tcIR);
    codegen->substitute(control->headers, parserHeaders);

    if (type != EBPF::TC_EGRESS && type != EBPF::XDP_EGRESS) {
        codegen->useAsPointerVariable(control->outputStandardMetadata->name.name);
    }

    codegen->useAsPointerVariable(control->headers->name.name);
    codegen->useAsPointerVariable(control->user_metadata->name.name);

    control->codeGen = codegen;

    for (auto a : ctrl->constantValue) {
        auto b = a.second;
        if (auto *block = b->to<IR::Block>()) {
            this->visit(block);
        }
    }
    return true;
}

bool ConvertToEBPFControlPNA::preorder(const IR::TableBlock *tblblk) {
    EBPFTablePNA *table = new EBPFTablePNA(program, tblblk, control->codeGen, tcIR);
    control->tables.emplace(tblblk->container->name, table);
    return true;
}

bool ConvertToEBPFControlPNA::checkPnaTimestampMem(const IR::Member *m) {
    if (m->expr != nullptr && m->expr->type != nullptr) {
        if (auto str_type = m->expr->type->to<IR::Type_Struct>()) {
            if (str_type->name == "pna_main_input_metadata_t" && m->member.name == "timestamp") {
                return true;
            }
        }
    }
    return false;
}

bool ConvertToEBPFControlPNA::preorder(const IR::Member *m) {
    // the condition covers timestamp field in pna_main_input_metadata_t
    if (checkPnaTimestampMem(m)) {
        control->timestampIsUsed = true;
    }
    return true;
}

bool ConvertToEBPFControlPNA::preorder(const IR::IfStatement *ifState) {
    if (ifState->condition->is<IR::Equ>()) {
        auto i = ifState->condition->to<IR::Equ>();
        if (auto rightMem = i->right->to<IR::Member>()) {
            if (checkPnaTimestampMem(rightMem)) control->timestampIsUsed = true;
        }
        if (auto leftMem = i->left->to<IR::Member>()) {
            if (checkPnaTimestampMem(leftMem)) control->timestampIsUsed = true;
        }
    }
    return true;
}

bool ConvertToEBPFControlPNA::preorder(const IR::Declaration_Variable *decl) {
    if (type == EBPF::TC_INGRESS || type == EBPF::XDP_INGRESS) {
        if (decl->type->is<IR::Type_Name>() &&
            decl->type->to<IR::Type_Name>()->path->name.name == "pna_main_output_metadata_t") {
            control->codeGen->useAsPointerVariable(decl->name.name);
        }
    }
    return true;
}

bool ConvertToEBPFControlPNA::preorder(const IR::ExternBlock *instance) {
    auto di = instance->node->to<IR::Declaration_Instance>();
    if (di == nullptr) return false;
    cstring name = EBPF::EBPFObject::externalName(di);
    cstring typeName = instance->type->getName().name;
    if (typeName == "Hash") {
        auto hash = new EBPFHashPNA(program, di, name);
        control->pna_hashes.emplace(name, hash);
    } else if (typeName == "Register") {
        auto reg = new EBPFRegisterPNA(program, name, di, control->codeGen);
        control->pna_registers.emplace(name, reg);
        control->addExternDeclaration = true;
    } else if (typeName == "DirectCounter") {
        control->addExternDeclaration = true;
        return false;
    } else if (typeName == "Counter") {
        control->addExternDeclaration = true;
        auto ctr = new EBPFCounterPNA(program, di, name, control->codeGen);
        control->counters.emplace(name, ctr);
    } else if (typeName == "DirectMeter") {
        control->addExternDeclaration = true;
    } else if (typeName == "Meter") {
        control->addExternDeclaration = true;
        auto met = new EBPFMeterPNA(program, name, di, control->codeGen);
        control->meters.emplace(name, met);
    } else {
        ::P4::error(ErrorType::ERR_UNEXPECTED, "Unexpected block %s nested within control",
                    instance);
    }

    return false;
}

// =====================ConvertToEBPFDeparserPNA=============================
bool ConvertToEBPFDeparserPNA::preorder(const IR::ControlBlock *ctrl) {
    deparser = new IngressDeparserPNA(program, ctrl, parserHeaders, istd);
    if (!deparser->build()) {
        BUG("failed to build deparser");
    }

    deparser->codeGen->substitute(deparser->headers, parserHeaders);
    deparser->codeGen->useAsPointerVariable(deparser->headers->name.name);

    deparser->codeGen->useAsPointerVariable(deparser->user_metadata->name.name);

    this->visit(ctrl->container);

    return false;
}

bool ConvertToEBPFDeparserPNA::preorder(const IR::Declaration_Instance *di) {
    if (auto typeSpec = di->type->to<IR::Type_Specialized>()) {
        auto baseType = typeSpec->baseType;
        auto typeName = baseType->to<IR::Type_Name>()->path->name.name;
        if (typeName == "Digest") {
            deparser->addExternDeclaration = true;
            cstring instance = EBPF::EBPFObject::externalName(di);
            auto digest = new EBPFDigestPNA(program, di, typeName, tcIR);

            if (digest->valueType->is<EBPF::EBPFStructType>()) {
                for (auto type : ebpfTypes) {
                    if (type->is<EBPF::EBPFStructType>()) {
                        auto structType = type->to<EBPF::EBPFStructType>();
                        auto digestType = digest->valueType->to<EBPF::EBPFStructType>();

                        if (digestType->name == structType->name) structType->packed = true;
                    }
                }
            }
            deparser->digests.emplace(instance, digest);
        }
    }

    return false;
}

// =====================ControlBodyTranslatorPNA=============================
ControlBodyTranslatorPNA::ControlBodyTranslatorPNA(const EBPFControlPNA *control)
    : EBPF::CodeGenInspector(control->program->refMap, control->program->typeMap),
      EBPF::ControlBodyTranslator(control) {}

ControlBodyTranslatorPNA::ControlBodyTranslatorPNA(const EBPFControlPNA *control,
                                                   const ConvertToBackendIR *tcIR)
    : EBPF::CodeGenInspector(control->program->refMap, control->program->typeMap),
      EBPF::ControlBodyTranslator(control),
      tcIR(tcIR) {}

bool ControlBodyTranslatorPNA::preorder(const IR::Member *m) {
    if ((m->expr != nullptr) && (m->expr->type != nullptr)) {
        if (auto st = m->expr->type->to<IR::Type_Struct>()) {
            if (st->name == "pna_main_input_metadata_t") {
                if (m->member.name == "input_port") {
                    builder->append("skb->ifindex");
                    return false;
                } else if (m->member.name == "parser_error") {
                    builder->append("compiler_meta__->parser_error");
                    return false;
                } else {
                    ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                                "%1%: this metadata field is not supported", m);
                }
            }
        }
    }
    return CodeGenInspector::preorder(m);
}

ControlBodyTranslatorPNA::ControlBodyTranslatorPNA(const EBPFControlPNA *control,
                                                   const ConvertToBackendIR *tcIR,
                                                   const EBPF::EBPFTablePSA *table)
    : EBPF::CodeGenInspector(control->program->refMap, control->program->typeMap),
      EBPF::ControlBodyTranslator(control),
      tcIR(tcIR),
      table(table) {}

cstring ControlBodyTranslatorPNA::getParamName(const IR::PathExpression *expr) {
    return expr->path->name.name;
}

bool ControlBodyTranslatorPNA::IsTableAddOnMiss(const IR::P4Table *table) {
    auto property = table->getBooleanProperty("add_on_miss"_cs);
    if (property && property->value) {
        return true;
    }
    return false;
}

const IR::P4Action *ControlBodyTranslatorPNA::GetAddOnMissHitAction(cstring actionName) {
    for (auto a : table->actionList->actionList) {
        auto adecl = control->program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->to<IR::P4Action>();
        if (action->name.originalName == actionName) {
            if (a->getAnnotation("defaultonly"_cs)) {
                ::P4::error(ErrorType::ERR_UNEXPECTED,
                            "add_entry hit action %1% cannot be annotated with defaultonly.",
                            actionName);
            }
            return action;
        }
    }
    ::P4::error(ErrorType::ERR_UNEXPECTED,
                "add_entry extern can only be applied for one of the hit action of table "
                "%1%. %2% is not hit action of table.",
                table->instanceName, actionName);
    return nullptr;
}

void ControlBodyTranslatorPNA::ValidateAddOnMissMissAction(const IR::P4Action *act) {
    if (!act) {
        ::P4::error(ErrorType::ERR_UNEXPECTED, "%1% add_entry extern can only be used in an action",
                    act);
    }
    const IR::P4Table *t = table->table->container;
    const IR::Expression *defaultAction = t->getDefaultAction();
    CHECK_NULL(defaultAction);
    auto defaultActionName = table->getActionNameExpression(defaultAction);
    CHECK_NULL(defaultActionName);
    if (defaultActionName->path->name.originalName != act->name.originalName) {
        ::P4::error(ErrorType::ERR_UNEXPECTED,
                    "add_entry extern can only be applied in default action of the table.");
    }
    if (!IsTableAddOnMiss(t)) {
        ::P4::warning(ErrorType::WARN_MISSING,
                      "add_entry extern can only be used in an action"
                      " of a table with property add_on_miss equals to true.");
    }
    ((ConvertToBackendIR *)tcIR)->updateAddOnMissTable(t);
}

void ControlBodyTranslatorPNA::processFunction(const P4::ExternFunction *function) {
    if (function->expr->method->toString() == "send_to_port" ||
        function->expr->method->toString() == "drop_packet") {
        if (function->expr->method->toString() != "drop_packet") {
            builder->emitIndent();
            builder->appendLine("compiler_meta__->drop = false;");
        }
        builder->emitIndent();
        visit(function->expr->method);
        builder->append("(");
        bool first = true;
        for (auto a : *function->expr->arguments) {
            if (!first) builder->append(", ");
            first = false;
            visit(a);
        }
        builder->append(")");
        return;
    } else if (function->expr->method->toString() == "set_entry_expire_time") {
        if (table) {
            builder->emitIndent();
            builder->appendLine("/* construct key */");
            builder->emitIndent();
            builder->appendLine(
                "struct p4tc_table_entry_create_bpf_params__local update_params = {");
            builder->emitIndent();
            builder->appendLine("    .pipeid = p4tc_filter_fields.pipeid,");
            builder->emitIndent();
            auto controlName = control->controlBlock->getName().originalName;
            /* Table instanceName is control_block_name + "_" + original table name.
            Truncating control name to get the table name.*/
            auto tableName = table->instanceName.substr(controlName.size() + 1);
            auto tblId = tcIR->getTableId(tableName);
            BUG_CHECK(tblId != 0, "Table ID not found");
            builder->appendFormat("    .tblid = %d,", tblId);
            builder->newline();
            builder->emitIndent();
            builder->append("    .profile_id = ");
            for (auto a : *function->expr->arguments) {
                visit(a);
            }
            builder->newline();
            builder->emitIndent();
            builder->appendLine("};");
            builder->emitIndent();
            builder->append(
                "bpf_p4tc_entry_update(skb, &update_params, sizeof(params), &key, sizeof(key))");
        }
        return;
    } else if (function->expr->method->toString() == "add_entry") {
        /*
        add_entry() to be called with the following restrictions:
        * Only from within an action
        * Only if the action is a default action of a table with property add_on_miss equal to true.
        * Only with an action name that is one of the hit actions of that same table. This action
        has parameters that are all directionless.
        * The type T is a struct containing one member for each directionless parameter of the hit
          action to be added. The member names must match the hit action parameter names, and their
          types must be the same as the corresponding hit action parameters.
        */
        auto act = findContext<IR::P4Action>();
        ValidateAddOnMissMissAction(act);
        BUG_CHECK(function->expr->arguments->size() == 3,
                  "%1%: expected 3 arguments in add_entry extern", function);

        auto action = function->expr->arguments->at(0);
        auto actionName = action->expression->to<IR::StringLiteral>()->value;
        auto param = function->expr->arguments->at(1)->expression;
        auto expire_time_profile_id = function->expr->arguments->at(2);
        auto controlName = control->controlBlock->getName().originalName;

        if (auto action = GetAddOnMissHitAction(actionName)) {
            builder->emitIndent();
            builder->appendLine("struct p4tc_table_entry_act_bpf update_act_bpf = {};");

            if (param->is<IR::StructExpression>()) {
                auto paramList = action->getParameters();
                auto components = param->to<IR::StructExpression>()->components;
                if (paramList->parameters.size() != components.size()) {
                    ::P4::error(ErrorType::ERR_UNEXPECTED,
                                "Action params in add_entry should be same as no of action "
                                "parameters. %1%",
                                action);
                }
                if (paramList->parameters.size() != 0) {
                    builder->emitIndent();
                    builder->appendFormat(
                        "struct %s *update_act_bpf_val = (struct %s*) &update_act_bpf;",
                        table->valueTypeName.c_str(), table->valueTypeName.c_str());
                    builder->newline();
                    cstring actionExtName = EBPF::EBPFObject::externalName(action);
                    // Assign Variables
                    for (size_t index = 0; index < components.size(); index++) {
                        auto param = paramList->getParameter(index);
                        if (param->direction != IR::Direction::None) {
                            ::P4::error(ErrorType::ERR_UNEXPECTED,
                                        "Parameters of action called from add_entry should be "
                                        "directionless. %1%",
                                        actionName);
                        }
                        builder->emitIndent();
                        cstring value = "update_act_bpf_val->u."_cs + actionExtName + "."_cs +
                                        param->toString();
                        auto valuetype = typeMap->getType(param, true);
                        emitTCAssignmentEndianessConversion(
                            valuetype, nullptr, components.at(index)->expression, value);
                        builder->endOfStatement();
                        builder->newline();
                    }

                    builder->emitIndent();
                    builder->appendFormat("update_act_bpf_val->action = %v;",
                                          table->p4ActionToActionIDName(action));
                    builder->newline();
                } else {
                    builder->emitIndent();
                    builder->appendFormat("update_act_bpf.act_id = %v;",
                                          table->p4ActionToActionIDName(action));
                    builder->newline();
                }

            } else {
                ::P4::error(ErrorType::ERR_UNEXPECTED,
                            "action parameters of add_entry extern should be a structure only. %1%",
                            param);
            }
        }
        builder->newline();
        builder->emitIndent();
        builder->appendLine("/* construct key */");
        builder->emitIndent();
        builder->appendLine("struct p4tc_table_entry_create_bpf_params__local update_params = {");
        builder->emitIndent();
        builder->appendLine("    .act_bpf = update_act_bpf,");
        builder->emitIndent();
        builder->appendLine("    .pipeid = p4tc_filter_fields.pipeid,");
        builder->emitIndent();
        builder->appendLine("    .handle = p4tc_filter_fields.handle,");
        builder->emitIndent();
        builder->appendLine("    .classid = p4tc_filter_fields.classid,");
        builder->emitIndent();
        builder->appendLine("    .chain = p4tc_filter_fields.chain,");
        builder->emitIndent();
        builder->appendLine("    .proto = p4tc_filter_fields.proto,");
        builder->emitIndent();
        builder->appendLine("    .prio = p4tc_filter_fields.prio,");
        builder->emitIndent();
        auto tableName = table->instanceName.substr(controlName.size() + 1);
        auto tblId = tcIR->getTableId(tableName);
        BUG_CHECK(tblId != 0, "Table ID not found");
        builder->appendFormat("    .tblid = %d,", tblId);
        builder->newline();
        builder->emitIndent();
        builder->append("    .profile_id = ");
        visit(expire_time_profile_id);
        builder->newline();
        builder->emitIndent();
        builder->appendLine("};");
        builder->emitIndent();
        builder->append(
            "bpf_p4tc_entry_create_on_miss(skb, &update_params, sizeof(update_params), &key, "
            "sizeof(key))");
        return;
    }

    auto fname = function->expr->method->toString();

    if (is_skb_meta_func(fname)) {
        if (fname == "skb_get_meta") {
            if (function->expr->arguments->size() != 0) {
                ::P4::error("skb_get_meta takes no arguments");
                return;
            }
            builder->emitIndent();
            builder->append("bpf_p4tc_skb_meta_get(skb,&sa->get,sizeof(sa->get))");
        } else if ((fname == "skb_set_tstamp") || (fname == "skb_set_mark") ||
                   (fname == "skb_set_tc_classid") || (fname == "skb_set_tc_index") ||
                   (fname == "skb_set_queue_mapping") || (fname == "skb_set_protocol")) {
            if (function->expr->arguments->size() != 1) {
                ::P4::error("%1% takes exactly one argument", fname);
                return;
            }
            builder->emitIndent();
            builder->appendFormat("bpf_p4tc_%s(skb,&sa->set,", fname.c_str());
            visit(function->expr->arguments->at(0));
            builder->append(")");
        } else if (fname == "skb_set_meta") {
            if (function->expr->arguments->size() != 0) {
                ::P4::error("skb_set_meta takes no arguments");
                return;
            }
            builder->emitIndent();
            builder->append("bpf_p4tc_skb_meta_set(skb,&sa->set,sizeof(sa->set))");
        } else {
            BUG_CHECK(0, "impossible method name in %1%", &__func__[0]);
            return;
        }
        (dynamic_cast<const EBPFControlPNA *>(control))->touched_skb_metadata = true;
        return;
    }
    processCustomExternFunction(function, EBPF::EBPFTypeFactory::instance);
}

void ControlBodyTranslatorPNA::processApply(const P4::ApplyMethod *method) {
    P4::ParameterSubstitution binding;
    cstring actionVariableName, msgStr;
    auto table = control->getTable(method->object->getName().name);
    BUG_CHECK(table != nullptr, "No table for %1%", method->expr);

    msgStr = absl::StrFormat("Control: applying %v", method->object->getName());
    builder->target->emitTraceMessage(builder, msgStr.c_str());

    builder->emitIndent();

    if (!saveAction.empty()) {
        actionVariableName = saveAction.at(saveAction.size() - 1);
        if (!actionVariableName.isNullOrEmpty()) {
            builder->appendFormat("unsigned int %s = 0;\n", actionVariableName.c_str());
            builder->emitIndent();
        }
    }
    builder->blockStart();

    BUG_CHECK(method->expr->arguments->size() == 0, "%1%: table apply with arguments", method);
    cstring keyname = "key"_cs;
    if (table->keyGenerator != nullptr) {
        builder->emitIndent();
        builder->appendLine("/* construct key */");
        builder->emitIndent();
        builder->appendLine("struct p4tc_table_entry_act_bpf_params__local params = {");
        builder->emitIndent();
        builder->appendLine("    .pipeid = p4tc_filter_fields.pipeid,");
        builder->emitIndent();
        auto tblId = tcIR->getTableId(method->object->getName().originalName);
        BUG_CHECK(tblId != 0, "Table ID not found");
        builder->appendFormat("    .tblid = %d", tblId);
        builder->newline();
        builder->emitIndent();
        builder->appendLine("};");
        builder->emitIndent();
        builder->appendFormat("struct %s %s", table->keyTypeName.c_str(), keyname.c_str());
        builder->endOfStatement(true);
        builder->emitIndent();
        builder->appendFormat("__builtin_memset(&%s, 0, sizeof(%s))", keyname.c_str(),
                              keyname.c_str());
        builder->endOfStatement(true);
        unsigned int keysize = tcIR->getTableKeysize(tblId);
        builder->emitIndent();
        builder->appendFormat("%s.keysz = %d;", keyname.c_str(), keysize);
        builder->newline();
        // Emit Keys
        for (auto c : table->keyGenerator->keyElements) {
            auto ebpfType = ::P4::get(table->keyTypes, c);
            cstring fieldName = ::P4::get(table->keyFieldNames, c);
            if (fieldName == nullptr || ebpfType == nullptr) continue;
            bool memcpy = false;
            EBPF::EBPFScalarType *scalar = nullptr;
            cstring swap;

            auto tcTarget = dynamic_cast<const EBPF::P4TCTarget *>(builder->target);
            cstring isKeyBigEndian = tcTarget->getByteOrderFromAnnotation(c);
            cstring isDefnBigEndian = "HOST"_cs;
            if (auto mem = c->expression->to<IR::Member>()) {
                auto type = typeMap->getType(mem->expr, true);
                if (type->is<IR::Type_StructLike>()) {
                    auto field = type->to<IR::Type_StructLike>()->getField(mem->member);
                    isDefnBigEndian = tcTarget->getByteOrderFromAnnotation(field);
                }
            }

            bool primitive = true;
            if (ebpfType->is<EBPF::EBPFScalarType>()) {
                scalar = ebpfType->to<EBPF::EBPFScalarType>();
                unsigned width = scalar->implementationWidthInBits();
                memcpy = !EBPF::EBPFScalarType::generatesScalar(width);
                auto tcTarget = dynamic_cast<const EBPF::P4TCTarget *>(builder->target);
                primitive = tcTarget->isPrimitiveByteAligned(scalar->implementationWidthInBits());

                if (isKeyBigEndian == "NETWORK" && isDefnBigEndian == "HOST") {
                    if (width <= 8) {
                        swap = ""_cs;  // single byte, nothing to swap
                    } else if (width <= 16) {
                        swap = "bpf_htons"_cs;
                    } else if (width <= 32) {
                        swap = "bpf_htonl"_cs;
                    } else if (width <= 64) {
                        swap = "bpf_htonll"_cs;
                    }
                    /* For width greater than 64 bit, there is no need of conversion.
                        and the value will be copied directly from memory.*/
                }
            }

            builder->emitIndent();
            if (!primitive) {
                builder->appendFormat("__builtin_memcpy(&(%v.%v), &(", keyname, fieldName);
                table->codeGen->visit(c->expression);
                builder->appendFormat("), %d)", scalar->bytesRequired());
            } else if (memcpy) {
                builder->appendFormat("__builtin_memcpy(&(%v.%v[0]), &(", keyname, fieldName);
                table->codeGen->visit(c->expression);
                builder->appendFormat("[0]), %d)", scalar->bytesRequired());
            } else {
                builder->appendFormat("%v.%v = ", keyname, fieldName);
                if (isKeyBigEndian == "NETWORK" && isDefnBigEndian == "HOST")
                    builder->appendFormat("%v(", swap);
                table->codeGen->visit(c->expression);
                if (isKeyBigEndian == "NETWORK" && isDefnBigEndian == "HOST") builder->append(")");
            }
            builder->endOfStatement(true);

            cstring msgStr, varStr;
            if (memcpy) {
                msgStr = absl::StrFormat("Control: key %v", c->expression);
                builder->target->emitTraceMessage(builder, msgStr.c_str());
            } else {
                msgStr = absl::StrFormat("Control: key %v=0x%%llx", c->expression);
                varStr = absl::StrFormat("(unsigned long long) %s.%s", keyname.c_str(),
                                         fieldName.c_str());
                builder->target->emitTraceMessage(builder, msgStr.c_str(), 1, varStr.c_str());
            }
        }
    }

    builder->emitIndent();
    builder->appendLine("struct p4tc_table_entry_act_bpf *act_bpf;");

    builder->emitIndent();
    builder->appendLine("/* value */");
    builder->emitIndent();
    cstring valueName = "value"_cs;
    builder->appendFormat("struct %s *%s = NULL", table->valueTypeName.c_str(), valueName.c_str());
    builder->endOfStatement(true);

    if (table->keyGenerator != nullptr) {
        builder->emitIndent();
        builder->appendLine("/* perform lookup */");
        builder->target->emitTraceMessage(builder, "Control: performing table lookup");
        builder->emitIndent();
        builder->appendLine(
            "act_bpf = bpf_p4tc_tbl_read(skb, &params, sizeof(params), &key, sizeof(key));");
        builder->emitIndent();
        builder->appendFormat("value = (struct %s *)act_bpf;", table->valueTypeName.c_str());
        builder->newline();
    }

    builder->emitIndent();
    builder->appendFormat("if (%s == NULL) ", valueName.c_str());
    builder->blockStart();

    builder->emitIndent();
    builder->appendLine("/* miss; find default action */");
    builder->target->emitTraceMessage(builder, "Control: Entry not found, going to default action");
    builder->emitIndent();
    builder->appendFormat("%s = 0", control->hitVariable.c_str());
    builder->endOfStatement(true);

    builder->blockEnd(false);
    builder->append(" else ");
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("%s = value->hit", control->hitVariable.c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);

    if (table->cacheEnabled()) {
        table->emitCacheUpdate(builder, keyname, valueName);
        builder->blockEnd(true);
    }

    builder->emitIndent();
    builder->appendFormat("if (%s != NULL) ", valueName.c_str());
    table->emitAction(builder, valueName, actionVariableName);
    toDereference.clear();
    builder->blockEnd(true);

    msgStr = absl::StrFormat("Control: %v applied", method->object->getName());
    builder->target->emitTraceMessage(builder, msgStr.c_str());
}

static void gen_skb_call_ctrl(const EBPFControlPNA *c, EBPF::CodeBuilder *b,
                              const P4::ExternMethod *m, ControlBodyTranslatorPNA *xlat) {
    EBPF::CodeGenInspector *inspector = xlat;

    c->touched_skb_metadata = true;
    gen_skb_call(b, m, inspector);
}

void ControlBodyTranslatorPNA::processMethod(const P4::ExternMethod *method) {
    auto decl = method->object;
    auto declType = method->originalExternType;
    cstring name = EBPF::EBPFObject::externalName(decl);
    auto pnaControl = dynamic_cast<const EBPFControlPNA *>(control);

    if (declType->name.name == "Register") {
        auto reg = pnaControl->getRegister(name);
        if (method->method->type->name == "write") {
            reg->emitRegisterWrite(builder, method, this);
        } else if (method->method->type->name == "read") {
            ::P4::warning(ErrorType::WARN_UNUSED, "This Register(%1%) read value is not used!",
                          name);
            reg->emitRegisterRead(builder, method, this, nullptr);
        }
        return;
    } else if (declType->name.name == "Counter") {
        auto counterMap = control->getCounter(name);
        auto pna_ctr = dynamic_cast<EBPFCounterPNA *>(counterMap);
        pna_ctr->emitMethodInvocation(builder, method, this);
        return;
    } else if (declType->name.name == "Meter") {
        auto meter = control->to<EBPF::EBPFControlPSA>()->getMeter(name);
        auto pna_meter = dynamic_cast<EBPFMeterPNA *>(meter);
        ::P4::warning(ErrorType::WARN_UNUSED, "This Meter (%1%) return value is not used!", name);
        pna_meter->emitExecute(builder, method, this, nullptr);
        return;
    } else if (declType->name.name == "Hash") {
        auto hash = pnaControl->getHash(name);
        hash->processMethod(builder, method->method->name.name, method->expr, this);
        return;
    } else if (declType->name.name == "tc_skb_metadata") {
        gen_skb_call_ctrl(pnaControl, builder, method, this);
        return;
    } else {
        ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "%1%: [C] [%2%] Unexpected method call",
                    method->expr, declType->name.name);
    }
}

bool ControlBodyTranslatorPNA::preorder(const IR::AssignmentStatement *a) {
    if (auto methodCallExpr = a->right->to<IR::MethodCallExpression>()) {
        auto mname = methodCallExpr->method->toString();
        if (mname == "is_net_port" || mname == "is_host_port") {
            builder->emitIndent();
            visit(a->left);
            builder->append(" = ");
            if (methodCallExpr->method->toString() == "is_net_port") {
                builder->append("bpf_p4tc_is_net_port(skb, ");
            } else {
                builder->append("bpf_p4tc_is_host_port(skb, ");
            }
            assert(methodCallExpr->arguments->size() == 1);
            visit(methodCallExpr->arguments->at(0));
            builder->append(");");
        } else if ((mname == "skb_get_tstamp") || (mname == "skb_get_mark") ||
                   (mname == "skb_get_tc_classid") || (mname == "skb_get_tc_index") ||
                   (mname == "skb_get_queue_mapping") || (mname == "skb_get_protocol") ||
                   (mname == "skb_get_tc_at_ingress") || (mname == "skb_get_from_ingress")) {
            if (methodCallExpr->arguments->size() != 0) {
                ::P4::error("%1% takes no arguments", mname);
                return (false);
            }
            visit(a->left);
            builder->appendFormat(" = bpf_p4tc_%s(skb, &sa->get);", mname.c_str());
            return (false);
        }
        auto mi = P4::MethodInstance::resolve(methodCallExpr, control->program->refMap,
                                              control->program->typeMap);
        auto ext = mi->to<P4::ExternMethod>();
        auto pnaControl = dynamic_cast<const EBPFControlPNA *>(control);
        if (ext == nullptr) {
            return false;
        }
        if (ext->originalExternType->name.name == "Register" && ext->method->type->name == "read") {
            cstring name = EBPF::EBPFObject::externalName(ext->object);
            auto reg = pnaControl->getRegister(name);
            reg->emitRegisterRead(builder, ext, this, a->left);
            return false;
        } else if (ext->originalExternType->name.name == "Hash") {
            cstring name = EBPF::EBPFObject::externalName(ext->object);
            auto hash = pnaControl->getHash(name);
            // Before assigning a value to a left expression we have to calculate a hash.
            // Then the hash value is stored in a registerVar variable.
            hash->calculateHash(builder, ext->expr, this);
            builder->emitIndent();
        } else if (ext->originalExternType->name.name == "Meter") {
            cstring name = EBPF::EBPFObject::externalName(ext->object);
            auto meter = control->to<EBPF::EBPFControlPSA>()->getMeter(name);
            auto pna_meter = dynamic_cast<EBPFMeterPNA *>(meter);
            pna_meter->emitExecute(builder, ext, this, a->left);
            return false;
        } else if (ext->originalExternType->name.name == "DirectMeter") {
            cstring name = EBPF::EBPFObject::externalName(ext->object);
            auto meter = table->getMeter(name);
            auto pna_meter = dynamic_cast<EBPFMeterPNA *>(meter);
            pna_meter->emitDirectMeterExecute(builder, ext, this, a->left);
            return false;
        } else {
            return (false);
        }
    }
    auto ltype = typeMap->getType(a->left);
    if (ltype) {
        auto et = EBPF::EBPFTypeFactory::instance->create(ltype);
        if (et->is<EBPF::EBPFScalarType>()) {
            auto bits = et->to<EBPF::EBPFScalarType>()->implementationWidthInBits();
            auto tcTarget = dynamic_cast<const EBPF::P4TCTarget *>(builder->target);
            bool primitive = tcTarget->isPrimitiveByteAligned(bits);
            if (!primitive) {
                if (bits <= 32) {
                    builder->appendFormat("storePrimitive32((u8 *)&");
                } else if (bits <= 64) {
                    builder->appendFormat("storePrimitive64((u8 *)&");
                } else {
                    builder->appendFormat("assign_%u(&", bits);
                }
                visit(a->left);
                builder->append("[0], ");
                if (bits <= 64) {
                    cstring getPrimitive = bits < 32 ? "getPrimitive32"_cs : "getPrimitive64"_cs;

                    builder->appendFormat("%v, %v((u8 *)(", bits, getPrimitive);
                    visit(a->right);
                    builder->appendFormat("), %v)", bits);
                } else {
                    visitHostOrder(a->right);
                }

                builder->append(");");
                return (false);
            }
        }
    }

    builder->emitIndent();
    emitTCAssignmentEndianessConversion(ltype, a->left, a->right, nullptr);
    builder->endOfStatement();
    return false;
}
// =====================ActionTranslationVisitorPNA=============================
ActionTranslationVisitorPNA::ActionTranslationVisitorPNA(
    const EBPF::EBPFProgram *program, cstring valueName, const EBPF::EBPFTablePSA *table,
    const ConvertToBackendIR *tcIR, const IR::P4Action *act, bool isDefaultAction)
    : EBPF::CodeGenInspector(program->refMap, program->typeMap),
      EBPF::ActionTranslationVisitor(valueName, program),
      ControlBodyTranslatorPNA((const EBPFControlPNA *)program->as<EBPF::EBPFPipeline>().control,
                               tcIR, table),
      table(table),
      isDefaultAction(isDefaultAction) {
    this->tcIR = tcIR;
    action = act;
}

bool ActionTranslationVisitorPNA::preorder(const IR::PathExpression *pe) {
    if (isActionParameter(pe)) {
        return EBPF::ActionTranslationVisitor::preorder(pe);
    }
    return EBPF::ControlBodyTranslator::preorder(pe);
}

bool ActionTranslationVisitorPNA::isActionParameter(const IR::Expression *expression) const {
    if (auto path = expression->to<IR::PathExpression>())
        return EBPF::ActionTranslationVisitor::isActionParameter(path);
    else if (auto cast = expression->to<IR::Cast>())
        return isActionParameter(cast->expr);
    else
        return false;
}

cstring ActionTranslationVisitorPNA::getParamInstanceName(const IR::Expression *expression) const {
    if (auto cast = expression->to<IR::Cast>()) expression = cast->expr;

    if (isDefaultAction) {
        cstring actionName = action->name.originalName;
        auto paramStr =
            absl::StrFormat("p4tc_filter_fields.%v_%v_%v",
                            table->table->container->name.originalName, actionName, expression);
        return paramStr;
    } else {
        return EBPF::ActionTranslationVisitor::getParamInstanceName(expression);
    }
}

cstring ActionTranslationVisitorPNA::getParamName(const IR::PathExpression *expr) {
    if (isActionParameter(expr)) {
        return getParamInstanceName(expr);
    }
    return ControlBodyTranslatorPNA::getParamName(expr);
}

void ActionTranslationVisitorPNA::processMethod(const P4::ExternMethod *method) {
    auto declType = method->originalExternType;
    auto decl = method->object;
    BUG_CHECK(decl->is<IR::Declaration>(), "Extern has not been declared [A]: %1% (is a %2%)", decl,
              decl->node_type_name());
    auto instanceName = EBPF::EBPFObject::externalName(decl->to<IR::Declaration>());
    cstring name = EBPF::EBPFObject::externalName(decl);

    if (declType->name.name == "DirectCounter") {
        auto ctr = table->getDirectCounter(instanceName);
        auto pna_ctr = dynamic_cast<EBPFCounterPNA *>(ctr);
        if (pna_ctr != nullptr)
            pna_ctr->emitDirectMethodInvocation(builder, method, this->tcIR);
        else
            ::P4::error(ErrorType::ERR_NOT_FOUND,
                        "%1%: Table %2% does not own DirectCounter named %3%", method->expr,
                        table->table->container, instanceName);
    } else if (declType->name.name == "DirectMeter") {
        auto met = table->getMeter(instanceName);
        auto pna_meter = dynamic_cast<EBPFMeterPNA *>(met);
        if (pna_meter != nullptr) {
            ::P4::warning(ErrorType::WARN_UNUSED, "This Meter (%1%) return value is not used!",
                          name);
            pna_meter->emitDirectMeterExecute(builder, method, this, nullptr);
        } else {
            ::P4::error(ErrorType::ERR_NOT_FOUND,
                        "%1%: Table %2% does not own DirectMeter named %3%", method->expr,
                        table->table->container, instanceName);
        }
    } else {
        ControlBodyTranslatorPNA::processMethod(method);
    }
}

EBPF::ActionTranslationVisitor *EBPFTablePNA::createActionTranslationVisitor(
    cstring valueName, const EBPF::EBPFProgram *program, const IR::P4Action *action,
    bool isDefault) const {
    return new ActionTranslationVisitorPNA(program->checkedTo<EBPF::EBPFPipeline>(), valueName,
                                           this, tcIR, action, isDefault);
}

void EBPFTablePNA::validateKeys() const {
    if (keyGenerator == nullptr) return;

    auto lastKey = std::find_if(
        keyGenerator->keyElements.rbegin(), keyGenerator->keyElements.rend(),
        [](const IR::KeyElement *key) { return key->matchType->path->name.name != "selector"; });
    for (auto it : keyGenerator->keyElements) {
        auto mtdecl = program->refMap->getDeclaration(it->matchType->path, true);
        auto matchType = mtdecl->getNode()->checkedTo<IR::Declaration_ID>();
        if (matchType->name.name == P4::P4CoreLibrary::instance().lpmMatch.name) {
            if (it != *lastKey) {
                ::P4::error(ErrorType::ERR_UNSUPPORTED,
                            "%1% field key must be at the end of whole key", it->matchType);
            }
        }
    }
}

// =====================DeparserHdrEmitTranslatorPNA=============================
DeparserHdrEmitTranslatorPNA::DeparserHdrEmitTranslatorPNA(const EBPF::EBPFDeparser *deparser)
    : EBPF::CodeGenInspector(deparser->program->refMap, deparser->program->typeMap),
      EBPF::DeparserPrepareBufferTranslator(deparser),
      deparser(deparser) {
    setName("DeparserHdrEmitTranslatorPNA");
}

void DeparserHdrEmitTranslatorPNA::processMethod(const P4::ExternMethod *method) {
    // This method handles packet_out.emit() only and is intended to skip other externs
    if (method->method->name.name == p4lib.packetOut.emit.name) {
        auto decl = method->object;
        if (decl == deparser->packet_out) {
            auto expr = method->expr->arguments->at(0)->expression;
            auto exprType = deparser->program->typeMap->getType(expr);
            auto headerToEmit = exprType->to<IR::Type_Header>();
            if (headerToEmit == nullptr) {
                ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                            "Cannot emit a non-header type %1%", expr);
            }

            cstring msgStr;
            builder->emitIndent();
            builder->append("if (");
            this->visit(expr);
            builder->append(".ebpf_valid) ");
            builder->blockStart();
            auto program = deparser->program;
            unsigned width = headerToEmit->width_bits();
            msgStr = absl::StrFormat("Deparser: emitting header %s", expr->toString().c_str());
            builder->target->emitTraceMessage(builder, msgStr.c_str());

            builder->emitIndent();
            builder->appendFormat("if (%s < %s + BYTES(%s + %d)) ", program->packetEndVar.c_str(),
                                  program->packetStartVar.c_str(), program->offsetVar.c_str(),
                                  width);
            builder->blockStart();
            builder->target->emitTraceMessage(builder,
                                              "Deparser: invalid packet (packet too short)");
            builder->emitIndent();
            // We immediately return instead of jumping to reject state.
            // It avoids reaching BPF_COMPLEXITY_LIMIT_JMP_SEQ.
            builder->appendFormat("return %s;", builder->target->abortReturnCode().c_str());
            builder->newline();
            builder->blockEnd(true);
            builder->emitIndent();
            builder->newline();
            unsigned alignment = 0;
            for (auto f : headerToEmit->fields) {
                auto ftype = deparser->program->typeMap->getType(f);
                auto etype = EBPF::EBPFTypeFactory::instance->create(ftype);
                auto et = etype->to<EBPF::IHasWidth>();
                if (et == nullptr) {
                    ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                                "Only headers with fixed widths supported %1%", f);
                    return;
                }
                bool noEndiannessConversion = false;
                if (const auto *anno = f->getAnnotation(ParseTCAnnotations::tcType)) {
                    cstring value = anno->getExpr(0)->checkedTo<IR::StringLiteral>()->value;
                    noEndiannessConversion = value == "macaddr" || value == "ipv4" ||
                                             value == "ipv6" || value == "be16" ||
                                             value == "be32" || value == "be64";
                }
                emitField(builder, f->name, expr, alignment, etype, noEndiannessConversion);
                alignment += et->widthInBits();
                alignment %= 8;
            }
            builder->blockEnd(true);
        } else {
            BUG("emit() should only be invoked for packet_out");
        }
    }
}

void DeparserHdrEmitTranslatorPNA::emitField(EBPF::CodeBuilder *builder, cstring field,
                                             const IR::Expression *hdrExpr, unsigned int alignment,
                                             EBPF::EBPFType *type, bool noEndiannessConversion) {
    auto program = deparser->program;

    auto et = type->to<EBPF::IHasWidth>();
    if (et == nullptr) {
        ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                    "Only headers with fixed widths supported %1%", hdrExpr);
        return;
    }
    unsigned widthToEmit = et->widthInBits();
    unsigned emitSize = 0;
    cstring swap = ""_cs, msgStr;
    auto tcTarget = dynamic_cast<const EBPF::P4TCTarget *>(builder->target);
    bool isPrimitive = tcTarget->isPrimitiveByteAligned(widthToEmit);

    if (widthToEmit <= 64) {
        if (program->options.emitTraceMessages) {
            builder->emitIndent();
            builder->blockStart();
            builder->emitIndent();
            builder->append("u64 tmp = ");
            visit(hdrExpr);
            builder->appendFormat(".%s", field.c_str());
            builder->endOfStatement(true);
            msgStr = absl::StrFormat("Deparser: emitting field %v=0x%%llx (%u bits)", field,
                                     widthToEmit);
            builder->target->emitTraceMessage(builder, msgStr.c_str(), 1, "tmpAD");
            builder->blockEnd(true);
        }
    } else {
        msgStr = absl::StrFormat("Deparser: emitting field %v (%u bits)", field, widthToEmit);
        builder->target->emitTraceMessage(builder, msgStr.c_str());
    }

    if (widthToEmit <= 8) {
        emitSize = 8;
    } else if (widthToEmit <= 16) {
        swap = "bpf_htons"_cs;
        emitSize = 16;
    } else if (widthToEmit <= 32) {
        swap = "htonl"_cs;
        emitSize = 32;
    } else if (widthToEmit <= 64) {
        swap = "htonll"_cs;
        emitSize = 64;
    }
    unsigned shift =
        widthToEmit < 8 ? (emitSize - alignment - widthToEmit) : (emitSize - widthToEmit);

    if (!swap.isNullOrEmpty() && !noEndiannessConversion) {
        if (!isPrimitive) {
            builder->emitIndent();
            cstring storePrimitive =
                widthToEmit < 32 ? "storePrimitive32"_cs : "storePrimitive64"_cs;
            cstring getPrimitive = widthToEmit < 32 ? "getPrimitive32"_cs : "getPrimitive64"_cs;
            builder->appendFormat("%v((u8 *)&", storePrimitive);
            visit(hdrExpr);
            builder->appendFormat(".%v, %d, (%v(%v(", field, widthToEmit, swap, getPrimitive);
            visit(hdrExpr);
            builder->appendFormat(".%v, %d)", field, widthToEmit);
            if (shift != 0) builder->appendFormat(" << %d", shift);
            builder->append(")))");
            builder->endOfStatement(true);
        } else {
            builder->emitIndent();
            visit(hdrExpr);
            builder->appendFormat(".%v = %v(", field, swap);
            visit(hdrExpr);
            builder->appendFormat(".%v", field);
            if (shift != 0) builder->appendFormat(" << %d", shift);
            builder->append(")");
            builder->endOfStatement(true);
        }
    }
    unsigned bitsInFirstByte = widthToEmit % 8;
    if (bitsInFirstByte == 0) bitsInFirstByte = 8;
    unsigned bitsInCurrentByte = bitsInFirstByte;
    unsigned left = widthToEmit;
    for (unsigned i = 0; i < (widthToEmit + 7) / 8; i++) {
        builder->emitIndent();
        builder->appendFormat("%s = ((char*)(&", program->byteVar.c_str());
        visit(hdrExpr);
        builder->appendFormat(".%v))[%d]", field, i);
        builder->endOfStatement(true);
        unsigned freeBits = alignment != 0 ? (8 - alignment) : 8;
        bitsInCurrentByte = left >= 8 ? 8 : left;
        unsigned bitsToWrite = bitsInCurrentByte > freeBits ? freeBits : bitsInCurrentByte;
        BUG_CHECK((bitsToWrite > 0) && (bitsToWrite <= 8), "invalid bitsToWrite %d", bitsToWrite);
        builder->emitIndent();
        if (alignment == 0 && bitsToWrite == 8) {  // write whole byte
            builder->appendFormat("write_byte(%s, BYTES(%s) + %d, (%s))",
                                  program->packetStartVar.c_str(), program->offsetVar.c_str(),
                                  i,  // do not reverse byte order
                                  program->byteVar.c_str());
        } else {  // write partial
            shift = (8 - alignment - bitsToWrite);
            builder->appendFormat("write_partial(%s + BYTES(%s) + %d, %d, %d, (%s >> %d))",
                                  program->packetStartVar.c_str(), program->offsetVar.c_str(),
                                  i,  // do not reverse byte order
                                  bitsToWrite, shift, program->byteVar.c_str(),
                                  widthToEmit > freeBits ? alignment == 0 ? shift : alignment : 0);
        }
        builder->endOfStatement(true);
        left -= bitsToWrite;
        bitsInCurrentByte -= bitsToWrite;
        alignment = (alignment + bitsToWrite) % 8;
        bitsToWrite = (8 - bitsToWrite);
        if (bitsInCurrentByte > 0) {
            builder->emitIndent();
            if (bitsToWrite == 8) {
                builder->appendFormat("write_byte(%s, BYTES(%s) + %d + 1, (%s << %d))",
                                      program->packetStartVar.c_str(), program->offsetVar.c_str(),
                                      i,  // do not reverse byte order
                                      program->byteVar.c_str(), 8 - alignment % 8);
            } else {
                builder->appendFormat("write_partial(%s + BYTES(%s) + %d + 1, %d, %d, (%s))",
                                      program->packetStartVar.c_str(), program->offsetVar.c_str(),
                                      i,  // do not reverse byte order
                                      bitsToWrite, 8 + alignment - bitsToWrite,
                                      program->byteVar.c_str());
            }
            builder->endOfStatement(true);
            left -= bitsToWrite;
        }
        alignment = (alignment + bitsToWrite) % 8;
    }
    builder->emitIndent();
    builder->appendFormat("%s += %d", program->offsetVar.c_str(), widthToEmit);
    builder->endOfStatement(true);
    builder->newline();
}

EBPF::EBPFHashAlgorithmPSA *EBPFHashAlgorithmTypeFactoryPNA::create(
    int type, const EBPF::EBPFProgram *program, cstring name) {
    if (type == EBPF::EBPFHashAlgorithmPSA::HashAlgorithm::CRC16) {
        return new HashAlgorithmPNA(program, name, 16);
    } else if (type == EBPF::EBPFHashAlgorithmPSA::HashAlgorithm::CRC32) {
        return new HashAlgorithmPNA(program, name, 32);
    }

    return nullptr;
}

EBPF::EBPFHashAlgorithmPSA *EBPFChecksumAlgorithmTypeFactoryPNA::create(
    int type, const EBPF::EBPFProgram *program, cstring name) {
    if (type == EBPF::EBPFHashAlgorithmPSA::HashAlgorithm::CRC16) {
        return new CRCChecksumAlgorithmPNA(program, name, 16);
    } else if (type == EBPF::EBPFHashAlgorithmPSA::HashAlgorithm::CRC32) {
        return new CRCChecksumAlgorithmPNA(program, name, 32);
    } else if (type == EBPF::EBPFHashAlgorithmPSA::HashAlgorithm::ONES_COMPLEMENT16 ||
               type == EBPF::EBPFHashAlgorithmPSA::HashAlgorithm::TARGET_DEFAULT) {
        return new InternetChecksumAlgorithmPNA(program, name);
    }

    return nullptr;
}

bool ControlBodyTranslatorPNA::preorder(const IR::Concat *e) {
    auto lt = e->left->type->to<IR::Type_Bits>();
    if (lt) {
        auto rt = e->right->type->to<IR::Type_Bits>();
        if (rt) {
            auto lbits = lt->width_bits();
            auto rbits = rt->width_bits();
            if (lbits + rbits > 64) {
                builder->appendFormat("concat_%d_%d(", lbits, rbits);
                getBitAlignment(e->left);
                builder->append(",");
                getBitAlignment(e->right);
                builder->append(")");
            } else {
                builder->append("(((");
                getBitAlignment(e->left);
                builder->appendFormat(")<<%d)|(", rbits);
                getBitAlignment(e->right);
                builder->append("))");
            }
            return (false);
        }
    }
    return (false);
}

bool ControlBodyTranslatorPNA::preorder(const IR::Constant *expression) {
    unsigned width = EBPF::EBPFInitializerUtils::ebpfTypeWidth(typeMap, expression);

    if (EBPF::EBPFScalarType::generatesScalar(width)) {
        builder->append(Util::toString(expression->value, 0, false, expression->base));
        return true;
    }

    cstring str = EBPF::EBPFInitializerUtils::genHexStr(expression->value, width, expression);
    builder->appendFormat("(struct internal_bit_%u){{", width);
    for (size_t i = 0; i < str.size() / 2; ++i)
        builder->appendFormat("%s 0x%v", i ? "," : "", str.substr(2 * i, 2));
    builder->append(" }}");

    return false;
}

bool ControlBodyTranslatorPNA::arithCommon(const IR::Operation_Binary *e, const char *smallop,
                                           const char *bigop) {
    auto lt = e->left->type->to<IR::Type_Bits>();
    if (lt) {
        auto rt = e->right->type->to<IR::Type_Bits>();
        if (rt) {
            auto lbits = lt->width_bits();
            if (lbits <= 64) {
                visitHostOrder(e->left);
                builder->appendFormat(" %s ", smallop);
                visitHostOrder(e->right);
            } else {
                builder->appendFormat("%s_%u(", bigop, lbits);
                visitHostOrder(e->left);
                builder->append(", ");
                visitHostOrder(e->right);
                builder->append(")");
            }
            return (false);
        }
    }
    return (false);
}

bool ControlBodyTranslatorPNA::sarithCommon(const IR::Operation_Binary *e, const char *op) {
    auto lt = e->left->type->to<IR::Type_Bits>();
    if (lt) {
        auto rt = e->right->type->to<IR::Type_Bits>();
        auto lbits = lt->width_bits();
        if (rt) {
            assert((lt->width_bits() == rt->width_bits()) && (lt->isSigned == rt->isSigned));
            builder->appendFormat("%s_%u(", op, lbits);
            visitHostOrder(e->left);
            builder->append(", ");
            visitHostOrder(e->right);
            builder->append(")");
            return (false);
        }
    }
    return (false);
}

bool ControlBodyTranslatorPNA::bigXSmallMul(const IR::Expression *big, const IR::Constant *small) {
    assert(big->type->is<IR::Type_Bits>());
    auto bw = big->type->to<IR::Type_Bits>()->width_bits();
    assert(bw > 64);
    builder->appendFormat("bxsmul_%d_%u(", bw, static_cast<unsigned int>(small->value));
    getBitAlignment(big);
    builder->appendFormat(")");
    return (false);
}

bool ControlBodyTranslatorPNA::preorder(const IR::Add *e) { return (arithCommon(e, "+", "add")); }

bool ControlBodyTranslatorPNA::preorder(const IR::Sub *e) { return (arithCommon(e, "-", "sub")); }

bool ControlBodyTranslatorPNA::preorder(const IR::Mul *e) {
    const IR::Constant *cx;

    cx = e->left->to<IR::Constant>();
    if (cx && (cx->value >= 0) && (cx->value < (1U << 22))) {
        auto rt = e->right->type->to<IR::Type_Bits>();
        if (rt && (rt->width_bits() > 64)) {
            return (bigXSmallMul(e->right, cx));
        }
    }
    cx = e->right->to<IR::Constant>();
    if (cx && (cx->value >= 0) && (cx->value < (1U << 22))) {
        auto rt = e->left->type->to<IR::Type_Bits>();
        if (rt && (rt->width_bits() > 64)) {
            return (bigXSmallMul(e->left, cx));
        }
    }
    return (arithCommon(e, "*", "mul"));
}

bool ControlBodyTranslatorPNA::preorder(const IR::Cast *e) {
    assert(e->type->is<IR::Type_Bits>());
    assert(e->expr->type->is<IR::Type_Bits>());
    auto fw = e->expr->type->to<IR::Type_Bits>()->width_bits();
    auto tw = e->type->to<IR::Type_Bits>()->width_bits();
    if (fw == tw) {
        visit(e->expr);
        return (false);
    }
    if ((fw <= 64) && (tw <= 64)) {
        builder->append("(u64)(");
        getBitAlignment(e->expr);
        if (tw < fw) builder->appendFormat(" & %lluu", (1ULL << tw) - 1ULL);
        builder->append(")");
    } else {
        const IR::Expression *exp;
        if (auto cast = e->expr->to<IR::Cast>())
            exp = cast->expr;
        else
            exp = e->expr;

        builder->appendFormat("cast_%d_to_%d(", fw, tw);
        /* The cast function to a value of size >= 64-bits expects the argument
         * to be in host endian, so we need to convert network endian values
         */
        visitHostOrder(e->expr);
        builder->append(")");
    }
    return (false);
}

bool ControlBodyTranslatorPNA::preorder(const IR::Neg *e) {
    assert(e->type->is<IR::Type_Bits>());
    auto w = e->type->to<IR::Type_Bits>()->width_bits();
    if (w <= 64) return (static_cast<EBPF::CodeGenInspector *>(this)->preorder(e));

    builder->appendFormat("neg_%d(", w);
    getBitAlignment(e->expr);
    builder->append(")");
    return (false);
}

bool ControlBodyTranslatorPNA::preorder(const IR::Cmpl *e) {
    assert(e->type->is<IR::Type_Bits>());
    auto w = e->type->to<IR::Type_Bits>()->width_bits();
    if (w <= 64) return (static_cast<EBPF::CodeGenInspector *>(this)->preorder(e));

    builder->appendFormat("not_%d(", w);
    getBitAlignment(e->expr);
    builder->append(")");
    return (false);
}

bool ControlBodyTranslatorPNA::preorder(const IR::Shl *e) {
    assert(e->type->is<IR::Type_Bits>());
    auto lw = e->type->to<IR::Type_Bits>()->width_bits();
    auto ec = e->right->to<IR::Constant>();
    if (ec) {
        if (lw <= 64) {
            return (static_cast<EBPF::CodeGenInspector *>(this)->preorder(e));
        } else if (ec->value < lw) {
            builder->appendFormat("shl_%u_c_%u(", lw, static_cast<unsigned int>(ec->value));
            getBitAlignment(e->left);
            builder->append(")");
        } else {
            builder->appendFormat("(struct internal_bit_%u){0}", lw);
        }
    } else if (e->right->type->is<IR::Type_Bits>()) {
        auto rw = e->right->type->to<IR::Type_Bits>()->width_bits();
        if ((lw <= 64) && (rw <= 64)) {
            return (static_cast<EBPF::CodeGenInspector *>(this)->preorder(e));
        } else {
            builder->appendFormat("shl_%d_x_%d(", lw, rw);
            getBitAlignment(e->left);
            builder->append(",");
            getBitAlignment(e->right);
            builder->append(")");
        }
    } else {
        return (static_cast<EBPF::CodeGenInspector *>(this)->preorder(e));
    }
    return (false);
}

bool ControlBodyTranslatorPNA::preorder(const IR::Shr *e) {
    assert(e->type->is<IR::Type_Bits>());
    auto lw = e->type->to<IR::Type_Bits>()->width_bits();
    auto signc = e->type->to<IR::Type_Bits>()->isSigned ? 'a' : 'l';
    auto ec = e->right->to<IR::Constant>();
    if (ec) {
        if (lw <= 64) {
            return (static_cast<EBPF::CodeGenInspector *>(this)->preorder(e));
        } else if (ec->value < lw) {
            builder->appendFormat("shr%c_%u_c_%u(", signc, lw,
                                  static_cast<unsigned int>(ec->value));
            getBitAlignment(e->left);
            builder->append(")");
        } else {
            builder->appendFormat("(struct internal_bit_%u){0}", lw);
        }
    } else if (e->right->type->is<IR::Type_Bits>()) {
        auto rw = e->right->type->to<IR::Type_Bits>()->width_bits();
        if ((lw <= 64) && (rw <= 64)) {
            return (static_cast<EBPF::CodeGenInspector *>(this)->preorder(e));
        } else {
            builder->appendFormat("shr%c_%d_x_%d(", signc, lw, rw);
            getBitAlignment(e->left);
            builder->append(",");
            getBitAlignment(e->right);
            builder->append(")");
        }
    } else {
        return (static_cast<EBPF::CodeGenInspector *>(this)->preorder(e));
    }
    return (false);
}

static void gen_cmp(ControlBodyTranslatorPNA *cbt, EBPF::CodeBuilder *bld,
                    const IR::Operation_Binary *e, const char *small, const char *large) {
    auto lt = e->left->type->to<IR::Type_Bits>();
    assert(lt);
    assert(e->right->type->to<IR::Type_Bits>());
    auto w = lt->width_bits();
    assert(w == e->right->type->to<IR::Type_Bits>()->width_bits());
    assert(lt->isSigned == e->right->type->to<IR::Type_Bits>()->isSigned);
    if (w <= 64) {
        switch (small[0]) {
            case '=':
            case '!':
                cbt->visit(e->left);
                bld->appendFormat("%s", small);
                cbt->visit(e->right);
                break;
            case '<':
            case '>':
                if (lt->isSigned) {
                    bool exact = false;
                    int cw = 0;

                    if (w < 8) {
                        cw = 8;
                        exact = false;
                    } else if (w == 8) {
                        cw = 8;
                        exact = true;
                    } else if (w < 16) {
                        cw = 16;
                        exact = false;
                    } else if (w == 16) {
                        cw = 16;
                        exact = true;
                    } else if (w < 32) {
                        cw = 32;
                        exact = false;
                    } else if (w == 32) {
                        cw = 32;
                        exact = true;
                    } else if (w < 64) {
                        cw = 64;
                        exact = false;
                    } else if (w == 64) {
                        cw = 64;
                        exact = true;
                    } else
                        ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                                    "Impossible width %u in gen_cmp", w);

                    bld->appendFormat("((i%d)(", cw);
                    cbt->visit(e->left);
                    if (exact)
                        bld->append("))");
                    else
                        bld->appendFormat(")<<%u)", cw - w);
                    bld->appendFormat("%s", small);
                    bld->appendFormat("((i%d)(", cw);
                    cbt->visit(e->right);
                    if (exact)
                        bld->append("))");
                    else
                        bld->appendFormat(")<<%u)", cw - w);
                } else {
                    cbt->visit(e->left);
                    bld->appendFormat("%s", small);
                    cbt->visit(e->right);
                }
                break;
            default:
                assert(!"Invalid call to gen_cmp");
                break;
        }
    } else {
        bld->appendFormat("cmp_%s_%u_%s(", lt->isSigned ? "s" : "u", w, large);
        cbt->visit(e->left);
        bld->append(',');
        cbt->visit(e->right);
        bld->append(')');
    }
}

bool ControlBodyTranslatorPNA::preorder(const IR::Equ *e) {
    gen_cmp(this, builder, e, "==", "eq");
    return (false);
}

bool ControlBodyTranslatorPNA::preorder(const IR::Neq *e) {
    gen_cmp(this, builder, e, "!=", "ne");
    return (false);
}

bool ControlBodyTranslatorPNA::preorder(const IR::Lss *e) {
    gen_cmp(this, builder, e, "<", "lt");
    return (false);
}

bool ControlBodyTranslatorPNA::preorder(const IR::Leq *e) {
    gen_cmp(this, builder, e, "<=", "le");
    return (false);
}

bool ControlBodyTranslatorPNA::preorder(const IR::Grt *e) {
    gen_cmp(this, builder, e, ">", "gt");
    return (false);
}

bool ControlBodyTranslatorPNA::preorder(const IR::Geq *e) {
    gen_cmp(this, builder, e, ">=", "ge");
    return (false);
}

bool ControlBodyTranslatorPNA::preorder(const IR::BAnd *e) {
    return (arithCommon(e, "&", "bitand"));
}

bool ControlBodyTranslatorPNA::preorder(const IR::BOr *e) { return (arithCommon(e, "|", "bitor")); }

bool ControlBodyTranslatorPNA::preorder(const IR::BXor *e) {
    return (arithCommon(e, "^", "bitxor"));
}

bool ControlBodyTranslatorPNA::preorder(const IR::AddSat *e) { return (sarithCommon(e, "addsat")); }

bool ControlBodyTranslatorPNA::preorder(const IR::SubSat *e) { return (sarithCommon(e, "subsat")); }

void ControlBodyTranslatorPNA::visitHostOrder(const IR::Expression *e) {
    auto bo = dynamic_cast<const EBPF::P4TCTarget *>(builder->target)
                  ->getByteOrder(typeMap, findContext<IR::P4Action>(), e);
    if (bo == "NETWORK") {
        emitAndConvertByteOrder(e, "HOST"_cs);
    } else {
        getBitAlignment(e);
    }
}

}  // namespace P4::TC
