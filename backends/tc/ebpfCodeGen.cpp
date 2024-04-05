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

namespace TC {

// =====================PNAEbpfGenerator=============================
void PNAEbpfGenerator::emitPNAIncludes(EBPF::CodeBuilder *builder) const {
    builder->appendLine("#include <stdbool.h>");
    builder->appendLine("#include <linux/if_ether.h>");
    builder->appendLine("#include \"pna.h\"");
}

cstring PNAEbpfGenerator::getProgramName() const {
    auto progName = options.file;
    auto filename = progName.findlast('/');
    if (filename) progName = filename;
    progName = progName.exceptLast(3);
    progName = progName.trim("/\t\n\r");
    return progName;
}

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
    builder->appendLine(
        "struct internal_metadata {\n"
        "    __u16 pkt_ether_type;\n"
        "} __attribute__((aligned(4)));");
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

    pipeline->parser->headerType->declare(builder, "cpumap_hdr", false);
    builder->endOfStatement(true);
    builder->emitIndent();
    auto user_md_type = pipeline->typeMap->getType(pipeline->control->user_metadata);
    BUG_CHECK(user_md_type != nullptr, "cannot declare user metadata");
    auto userMetadataType = EBPF::EBPFTypeFactory::instance->create(user_md_type);
    userMetadataType->declare(builder, "cpumap_usermeta", false);
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
        if (table->isTcMayOverride) {
            cstring tblName = table->getTableName();
            cstring defaultActionName = table->defaultMissAction->getActionName();
            defaultActionName = defaultActionName.substr(
                defaultActionName.find('/') - defaultActionName.begin() + 1,
                defaultActionName.size());
            for (auto param : table->defaultMissActionParams) {
                cstring paramName = param->paramDetail->getParamName();
                cstring placeholder = tblName + "_" + defaultActionName + "_" + paramName;
                cstring typeName = param->paramDetail->getParamType();
                builder->emitIndent();
                builder->appendFormat("%s %s", typeName, placeholder);
                builder->endOfStatement(true);
            }
        }
    }
}

void PNAEbpfGenerator::emitPipelineInstances(EBPF::CodeBuilder *builder) const {
    pipeline->parser->emitValueSetInstances(builder);
    pipeline->deparser->emitDigestInstances(builder);

    builder->target->emitTableDecl(builder, "hdr_md_cpumap", EBPF::TablePerCPUArray, "u32",
                                   "struct hdr_md", 2);
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
    builder->appendFormat("#include \"%s\"", headerFile);
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
        builder->target->emitTableDecl(builder, "xdp2tc_cpumap", EBPF::TablePerCPUArray, "u32",
                                       "u16", 1);
    }

    emitPipelineInstances(builder);
    builder->appendLine("REGISTER_END()");
    builder->newline();
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
    builder->appendFormat("#include \"%s\"", headerFile);
    builder->newline();
    builder->newline();
    builder->appendLine("struct p4tc_filter_fields p4tc_filter_fields;");
    builder->newline();
    pipeline->name = "tc-parse";
    pipeline->sectionName = "p4tc/parse";
    pipeline->functionName = pipeline->name.replace("-", "_") + "_func";
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
    EBPFHashAlgorithmTypeFactoryPNA::instance()->emitGlobals(builder);
}

// =====================TCIngressPipelinePNA=============================
void TCIngressPipelinePNA::emit(EBPF::CodeBuilder *builder) {
    cstring msgStr, varStr;

    // firstly emit process() in-lined function and then the actual BPF section.
    builder->append("static __always_inline");
    builder->spc();
    // FIXME: use Target to generate metadata type
    cstring func_name = (name == "tc-parse") ? "run_parser" : "process";
    builder->appendFormat(
        "int %s(%s *%s, %s %s *%s, "
        "struct pna_global_metadata *%s",
        func_name, builder->target->packetDescriptorType(), model.CPacketName.str(),
        parser->headerType->as<EBPF::EBPFStructType>().kind,
        parser->headerType->as<EBPF::EBPFStructType>().name, parser->headers->name.name,
        compilerGlobalMetadata);

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
        builder->appendFormat("return %s;", dropReturnCode());
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
        msgStr = Util::printf_format(
            "%s parser: parsing new packet, input_port=%%d, path=%%d, "
            "pkt_len=%%d",
            sectionName);
        varStr = Util::printf_format("%s->packet_path", compilerGlobalMetadata);
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
        msgStr = Util::printf_format("%s control: packet processing started", sectionName);
        builder->target->emitTraceMessage(builder, msgStr.c_str());
        control->emit(builder);
        builder->blockEnd(true);
        msgStr = Util::printf_format("%s control: packet processing finished", sectionName);
        builder->target->emitTraceMessage(builder, msgStr.c_str());

        // DEPARSER
        builder->emitIndent();
        builder->blockStart();
        msgStr = Util::printf_format("%s deparser: packet deparsing started", sectionName);
        builder->target->emitTraceMessage(builder, msgStr.c_str());
        deparser->emit(builder);
        msgStr = Util::printf_format("%s deparser: packet deparsing finished", sectionName);
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
        builder->appendFormat("int %s(%s *%s)", functionName,
                              builder->target->packetDescriptorType(), model.CPacketName.str());
        builder->spc();

        builder->blockStart();

        emitGlobalMetadataInitializer(builder);

        emitHeaderInstances(builder);
        builder->newline();

        builder->emitIndent();
        builder->appendFormat("int ret = %d;", actUnspecCode);
        builder->newline();
        builder->emitIndent();
        builder->appendFormat("ret = %s(skb, ", func_name);

        builder->appendFormat("(%s %s *) %s, %s);",
                              parser->headerType->as<EBPF::EBPFStructType>().kind,
                              parser->headerType->as<EBPF::EBPFStructType>().name,
                              parser->headers->name.name, compilerGlobalMetadata);

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
        builder->appendFormat("int %s(%s *%s)", functionName,
                              builder->target->packetDescriptorType(), model.CPacketName.str());
        builder->spc();

        builder->blockStart();

        builder->emitIndent();
        builder->appendFormat(
            "struct pna_global_metadata *%s = (struct pna_global_metadata *) skb->cb;",
            compilerGlobalMetadata);
        builder->newline();

        emitHeaderInstances(builder);
        builder->newline();

        builder->emitIndent();
        builder->appendFormat("int ret = %d;", actUnspecCode);
        builder->newline();
        builder->emitIndent();
        builder->appendFormat("ret = %s(skb, ", func_name);

        builder->appendFormat("(%s %s *) %s, %s);",
                              parser->headerType->as<EBPF::EBPFStructType>().kind,
                              parser->headerType->as<EBPF::EBPFStructType>().name,
                              parser->headers->name.name, compilerGlobalMetadata);

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
        "struct pna_global_metadata *%s = (struct pna_global_metadata *) skb->cb;",
        compilerGlobalMetadata);
    builder->newline();

    // if Traffic Manager decided to pass packet to the kernel stack earlier, send it up immediately
    builder->emitIndent();
    builder->append("if (compiler_meta__->pass_to_kernel == true) return TC_ACT_OK;");
    builder->newline();

    // workaround to make TC protocol-independent, DO NOT REMOVE
    builder->emitIndent();
    // replace ether_type only if a packet comes from XDP
    builder->appendFormat("if (!%s->recirculated) ", compilerGlobalMetadata);
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
    builder->appendFormat("if (!%s->drop && %s->egress_port == 0) ", compilerGlobalMetadata,
                          compilerGlobalMetadata);
    builder->blockStart();
    builder->target->emitTraceMessage(builder, "IngressTM: Sending packet up to the kernel stack");

    // Since XDP helper re-writes EtherType for packets other than IPv4 (e.g., ARP)
    // we cannot simply return TC_ACT_OK to pass the packet up to the kernel stack,
    // because the kernel stack would receive a malformed packet (with invalid skb->protocol).
    // The workaround is to send the packet back to the same interface. If we redirect,
    // the packet will be re-written back to the original format.
    // At the beginning of the pipeline we check if pass_to_kernel is true and,
    // if so, the program returns TC_ACT_OK.
    builder->emitIndent();
    builder->appendLine("compiler_meta__->pass_to_kernel = true;");
    builder->emitIndent();
    builder->append("return bpf_redirect(skb->ifindex, BPF_F_INGRESS)");
    builder->endOfStatement(true);
    builder->blockEnd(true);

    cstring eg_port = Util::printf_format("%s->egress_port", compilerGlobalMetadata);
    cstring cos =
        Util::printf_format("%s.class_of_service", control->outputStandardMetadata->name.name);
    builder->target->emitTraceMessage(
        builder, "IngressTM: Sending packet out of port %d with priority %d", 2, eg_port, cos);
    builder->emitIndent();
    builder->appendFormat("return bpf_redirect(%s->egress_port, 0)", compilerGlobalMetadata);
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
                          builder->target->dataOffset(model.CPacketName.str()).c_str());
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("u8* %s = %s;", headerStartVar.c_str(), packetStartVar.c_str());
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("void* %s = %s;", packetEndVar.c_str(),
                          builder->target->dataEnd(model.CPacketName.str()).c_str());
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
    auto annolist = field->getAnnotations()->annotations;
    for (auto anno : annolist) {
        if (anno->name != ParseTCAnnotations::tcType) continue;
        auto annoBody = anno->body;
        for (auto annoVal : annoBody) {
            if (annoVal->text == "macaddr" || annoVal->text == "ipv4" || annoVal->text == "ipv6") {
                noEndiannessConversion = true;
                break;
            } else if (annoVal->text == "be16" || annoVal->text == "be32" ||
                       annoVal->text == "be64") {
                std::string sInt = annoVal->text.substr(2).c_str();
                unsigned int width = std::stoi(sInt);
                if (widthToExtract != width) {
                    ::error("Width of the field doesnt match the annotation width. '%1%'", field);
                }
                noEndiannessConversion = true;
                break;
            }
        }
    }

    msgStr = Util::printf_format("Parser: extracting field %s", fieldName);
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
            builder->appendFormat(".%s, %s + BYTES(%s), %d)", fieldName,
                                  program->packetStartVar.c_str(), program->offsetVar.c_str(),
                                  widthToExtract / 8);
        } else {
            visit(expr);
            builder->appendFormat(".%s = (", fieldName);
            type->emit(builder);
            builder->appendFormat(")((%s(%s, BYTES(%s))", helper, program->packetStartVar.c_str(),
                                  program->offsetVar.c_str());
            if (shift != 0) builder->appendFormat(" >> %d", shift);
            builder->append(")");
            if (widthToExtract != loadSize) {
                builder->append(" & EBPF_MASK(");
                type->emit(builder);
                builder->appendFormat(", %d)", widthToExtract);
            }
            builder->append(")");
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
            ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
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
        cstring tmp = Util::printf_format("(unsigned long long) %s.%s", exprStr, fieldName);

        msgStr = Util::printf_format("Parser: extracted %s=0x%%llx (%u bits)", fieldName,
                                     widthToExtract);
        builder->target->emitTraceMessage(builder, msgStr.c_str(), 1, tmp.c_str());
    } else {
        msgStr = Util::printf_format("Parser: extracted %s (%u bits)", fieldName, widthToExtract);
        builder->target->emitTraceMessage(builder, msgStr.c_str());
    }

    builder->newline();
}

void PnaStateTranslationVisitor::compileLookahead(const IR::Expression *destination) {
    cstring msgStr = Util::printf_format("Parser: lookahead for %s %s",
                                         state->parser->typeMap->getType(destination)->toString(),
                                         destination->toString());
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
                ::error(ErrorType::ERR_UNSUPPORTED, "Match of type %1% not supported",
                        c->matchType);
            }

            auto ebpfType = ::get(keyTypes, c);
            cstring fieldName = ::get(keyFieldNames, c);

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
        builder->appendFormat("#define MAX_%s_MASKS %u", keyTypeName.toUpper(),
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
    emitDirectValueTypes(builder);

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

    if (isTernaryTable()) {
        builder->emitIndent();
        builder->append("__u32 priority;");
        builder->newline();
    }

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
    return Util::printf_format("%s_ACT_%s", tableInstance.toUpper(), actionName.toUpper());
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
        builder->appendFormat("case %s: ", actionName);
        builder->newline();
        builder->increaseIndent();

        msgStr = Util::printf_format("Control: executing action %s", name);
        builder->target->emitTraceMessage(builder, msgStr.c_str());
        for (auto param : *(action->parameters)) {
            auto etype = EBPF::EBPFTypeFactory::instance->create(param->type);
            unsigned width = etype->as<EBPF::IHasWidth>().widthInBits();

            if (width <= 64) {
                convStr = Util::printf_format("(unsigned long long) (%s->u.%s.%s)", valueName, name,
                                              param->toString());
                msgStr = Util::printf_format("Control: param %s=0x%%llx (%d bits)",
                                             param->toString(), width);
                builder->target->emitTraceMessage(builder, msgStr.c_str(), 1, convStr.c_str());
            } else {
                msgStr =
                    Util::printf_format("Control: param %s (%d bits)", param->toString(), width);
                builder->target->emitTraceMessage(builder, msgStr.c_str());
            }
        }
        bool generateDefaultMissCode = false;
        for (auto tbl : tcIR->tcPipeline->tableDefs) {
            if (tbl->getTableName() == table->container->name.originalName) {
                if (tbl->isTcMayOverride) {
                    cstring defaultActionName = tbl->defaultMissAction->getActionName();
                    defaultActionName = defaultActionName.substr(
                        defaultActionName.find('/') - defaultActionName.begin() + 1,
                        defaultActionName.size());
                    if (defaultActionName == action->name.originalName)
                        generateDefaultMissCode = true;
                }
            }
        }
        if (generateDefaultMissCode) {
            builder->emitIndent();
            builder->appendFormat("{");
            builder->newline();
            builder->increaseIndent();

            builder->emitIndent();
            builder->appendFormat("if (%s->is_default_miss_act) ", valueName.c_str());
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
            Util::printf_format("%s_ACT_%s", tableInstance.toUpper(), noActionName.toUpper());
        builder->emitIndent();
        builder->appendFormat("case %s: ", actionName);
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
        builder->appendFormat("#define %s %d", p4ActionToActionIDName(action), action_idx);
        builder->newline();
    }
    if (!noActionGenerated) {
        cstring noActionName = P4::P4CoreLibrary::instance().noAction.name;
        cstring tableInstance = dataMapName;
        cstring actionName =
            Util::printf_format("%s_ACT_%s", tableInstance.toUpper(), noActionName.toUpper());
        unsigned int action_idx = tcIR->getActionId(noActionName);
        builder->emitIndent();
        builder->appendFormat("#define %s %d", actionName, action_idx);
        builder->newline();
    }
    builder->emitIndent();
}

// =====================IngressDeparserPNA=============================
bool IngressDeparserPNA::build() {
    auto pl = controlBlock->container->type->applyParams;

    if (pl->size() != 4) {
        ::error(ErrorType::ERR_EXPECTED, "Expected deparser to have exactly 4 parameters");
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
    builder->appendFormat("if (%s->drop) ", pipeline->compilerGlobalMetadata);
    builder->blockStart();
    builder->target->emitTraceMessage(builder, "PreDeparser: dropping packet..");
    builder->emitIndent();
    builder->appendFormat("return %s;\n", builder->target->abortReturnCode().c_str());
    builder->blockEnd(true);
}

void IngressDeparserPNA::emit(EBPF::CodeBuilder *builder) {
    codeGen->setBuilder(builder);

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

    emitBufferAdjusts(builder);
    builder->emitIndent();
    builder->appendFormat("%s = %s;", program->packetStartVar,
                          builder->target->dataOffset(program->model.CPacketName.str()));
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("%s = %s;", program->packetEndVar,
                          builder->target->dataEnd(program->model.CPacketName.str()));
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
    auto pipelineParser = pipeline->getParameterValue("main_parser");
    BUG_CHECK(pipelineParser != nullptr, "No parser block found");
    auto pipelineControl = pipeline->getParameterValue("main_control");
    BUG_CHECK(pipelineControl != nullptr, "No control block found");
    auto pipelineDeparser = pipeline->getParameterValue("main_deparser");
    BUG_CHECK(pipelineDeparser != nullptr, "No deparser block found");

    auto xdp = new EBPF::XDPHelpProgram(options);

    auto pipeline_converter = new ConvertToEbpfPipelineTC(
        "tc-ingress", EBPF::TC_INGRESS, options, pipelineParser->checkedTo<IR::ParserBlock>(),
        pipelineControl->checkedTo<IR::ControlBlock>(),
        pipelineDeparser->checkedTo<IR::ControlBlock>(), refmap, typemap, tcIR);
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
    pipeline->sectionName = "p4tc/main";
    auto parser_converter = new ConvertToEBPFParserPNA(pipeline, typemap);
    parserBlock->apply(*parser_converter);
    pipeline->parser = parser_converter->getEBPFParser();
    CHECK_NULL(pipeline->parser);

    auto control_converter =
        new ConvertToEBPFControlPNA(pipeline, pipeline->parser->headers, refmap, type, tcIR);
    controlBlock->apply(*control_converter);
    pipeline->control = control_converter->getEBPFControl();
    CHECK_NULL(pipeline->control);

    auto deparser_converter = new ConvertToEBPFDeparserPNA(
        pipeline, pipeline->parser->headers, pipeline->control->outputStandardMetadata);
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
        ::error(ErrorType::ERR_EXPECTED, "Expected parser to have exactly %1% parameters",
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

// =====================ConvertToEBPFControlPNA=============================
bool ConvertToEBPFControlPNA::preorder(const IR::ControlBlock *ctrl) {
    control = new EBPF::EBPFControlPSA(program, ctrl, parserHeaders);
    program->control = control;
    program->as<EBPF::EBPFPipeline>().control = control;
    control->hitVariable = refmap->newName("hit");
    auto pl = ctrl->container->type->applyParams;
    unsigned numOfParams = 4;
    if (pl->size() != numOfParams) {
        ::error(ErrorType::ERR_EXPECTED, "Expected control to have exactly %1% parameters",
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
        auto hash = new EBPF::EBPFHashPSA(program, di, name);
        control->hashes.emplace(name, hash);
    } else {
        ::error(ErrorType::ERR_UNEXPECTED, "Unexpected block %s nested within control", instance);
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
            cstring instance = EBPF::EBPFObject::externalName(di);
            auto digest = new EBPF::EBPFDigestPSA(program, di);
            deparser->digests.emplace(instance, digest);
        }
    }

    return false;
}

// =====================ControlBodyTranslatorPNA=============================
ControlBodyTranslatorPNA::ControlBodyTranslatorPNA(const EBPF::EBPFControlPSA *control)
    : EBPF::CodeGenInspector(control->program->refMap, control->program->typeMap),
      EBPF::ControlBodyTranslator(control) {}

ControlBodyTranslatorPNA::ControlBodyTranslatorPNA(const EBPF::EBPFControlPSA *control,
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
                    ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                            "%1%: this metadata field is not supported", m);
                }
            }
        }
    }
    return CodeGenInspector::preorder(m);
}

ControlBodyTranslatorPNA::ControlBodyTranslatorPNA(const EBPF::EBPFControlPSA *control,
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
    auto property = table->getBooleanProperty("add_on_miss");
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
            auto annotations = a->getAnnotations();
            if (annotations && annotations->getSingle("defaultonly")) {
                ::error(ErrorType::ERR_UNEXPECTED,
                        "add_entry hit action %1% cannot be annotated with defaultonly.",
                        actionName);
            }
            return action;
        }
    }
    ::error(ErrorType::ERR_UNEXPECTED,
            "add_entry extern can only be applied for one of the hit action of table "
            "%1%. %2% is not hit action of table.",
            table->instanceName, actionName);
    return nullptr;
}

void ControlBodyTranslatorPNA::ValidateAddOnMissMissAction(const IR::P4Action *act) {
    if (!act) {
        ::error(ErrorType::ERR_UNEXPECTED, "%1% add_entry extern can only be used in an action",
                act);
    }
    const IR::P4Table *t = table->table->container;
    cstring tblname = t->name.originalName;
    const IR::Expression *defaultAction = t->getDefaultAction();
    CHECK_NULL(defaultAction);
    auto defaultActionName = table->getActionNameExpression(defaultAction);
    CHECK_NULL(defaultActionName);
    if (defaultActionName->path->name.originalName != act->name.originalName) {
        ::error(ErrorType::ERR_UNEXPECTED,
                "add_entry extern can only be applied in default action of the table.");
    }
    if (!IsTableAddOnMiss(t)) {
        ::warning(ErrorType::WARN_MISSING,
                  "add_entry extern can only be used in an action"
                  " of a table with property add_on_miss equals to true.");
    }
    tcIR->updateAddOnMissTable(tblname);
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
            builder->append("    .aging_ms = ");
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
                    ::error(ErrorType::ERR_UNEXPECTED,
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
                            ::error(ErrorType::ERR_UNEXPECTED,
                                    "Parameters of action called from add_entry should be "
                                    "directionless. %1%",
                                    actionName);
                        }
                        builder->emitIndent();
                        builder->appendFormat("update_act_bpf_val->u.%s.%s = ", actionExtName,
                                              param->toString());
                        visit(components.at(index)->expression);
                        builder->endOfStatement();
                        builder->newline();
                    }
                    builder->emitIndent();
                    builder->appendFormat("update_act_bpf_val->action = %s;",
                                          table->p4ActionToActionIDName(action));
                    builder->newline();
                } else {
                    builder->emitIndent();
                    builder->appendFormat("update_act_bpf.act_id = %s;",
                                          table->p4ActionToActionIDName(action));
                    builder->newline();
                }

            } else {
                ::error(ErrorType::ERR_UNEXPECTED,
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
            "bpf_p4tc_entry_create_on_miss(skb, &update_params, sizeof(params), &key, "
            "sizeof(key))");
        return;
    }
    processCustomExternFunction(function, EBPF::EBPFTypeFactory::instance);
}

void ControlBodyTranslatorPNA::processApply(const P4::ApplyMethod *method) {
    P4::ParameterSubstitution binding;
    cstring actionVariableName, msgStr;
    auto table = control->getTable(method->object->getName().name);
    BUG_CHECK(table != nullptr, "No table for %1%", method->expr);

    msgStr = Util::printf_format("Control: applying %s", method->object->getName().name);
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
    cstring keyname = "key";
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
            auto ebpfType = ::get(table->keyTypes, c);
            cstring fieldName = ::get(table->keyFieldNames, c);
            if (fieldName == nullptr || ebpfType == nullptr) continue;
            bool memcpy = false;
            EBPF::EBPFScalarType *scalar = nullptr;
            cstring swap;

            auto tcTarget = dynamic_cast<const EBPF::P4TCTarget *>(builder->target);
            cstring isKeyBigEndian =
                tcTarget->getByteOrderFromAnnotation(c->getAnnotations()->annotations);
            cstring isDefnBigEndian = "HOST";
            if (auto mem = c->expression->to<IR::Member>()) {
                auto type = typeMap->getType(mem->expr, true);
                if (type->is<IR::Type_StructLike>()) {
                    auto field = type->to<IR::Type_StructLike>()->getField(mem->member);
                    isDefnBigEndian =
                        tcTarget->getByteOrderFromAnnotation(field->getAnnotations()->annotations);
                }
            }

            if (ebpfType->is<EBPF::EBPFScalarType>()) {
                scalar = ebpfType->to<EBPF::EBPFScalarType>();
                unsigned width = scalar->implementationWidthInBits();
                memcpy = !EBPF::EBPFScalarType::generatesScalar(width);

                if (isKeyBigEndian == "NETWORK" && isDefnBigEndian == "HOST") {
                    if (width <= 8) {
                        swap = "";  // single byte, nothing to swap
                    } else if (width <= 16) {
                        swap = "bpf_htons";
                    } else if (width <= 32) {
                        swap = "bpf_htonl";
                    } else if (width <= 64) {
                        swap = "bpf_htonll";
                    }
                    /* For width greater than 64 bit, there is no need of conversion.
                        and the value will be copied directly from memory.*/
                }
            }

            builder->emitIndent();
            if (memcpy) {
                builder->appendFormat("__builtin_memcpy(&(%s.%s[0]), &(", keyname.c_str(),
                                      fieldName.c_str());
                table->codeGen->visit(c->expression);
                builder->appendFormat("[0]), %d)", scalar->bytesRequired());
            } else {
                builder->appendFormat("%s.%s = ", keyname.c_str(), fieldName.c_str());
                if (isKeyBigEndian == "NETWORK" && isDefnBigEndian == "HOST")
                    builder->appendFormat("%s(", swap.c_str());
                table->codeGen->visit(c->expression);
                if (isKeyBigEndian == "NETWORK" && isDefnBigEndian == "HOST") builder->append(")");
            }
            builder->endOfStatement(true);

            cstring msgStr, varStr;
            if (memcpy) {
                msgStr = Util::printf_format("Control: key %s", c->expression->toString());
                builder->target->emitTraceMessage(builder, msgStr.c_str());
            } else {
                msgStr = Util::printf_format("Control: key %s=0x%%llx", c->expression->toString());
                varStr = Util::printf_format("(unsigned long long) %s.%s", keyname.c_str(),
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
    cstring valueName = "value";
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

    msgStr = Util::printf_format("Control: %s applied", method->object->getName().name);
    builder->target->emitTraceMessage(builder, msgStr.c_str());
}

void ControlBodyTranslatorPNA::processMethod(const P4::ExternMethod *method) {
    auto decl = method->object;
    auto declType = method->originalExternType;
    cstring name = EBPF::EBPFObject::externalName(decl);

    if (declType->name.name == "Hash") {
        auto hash = control->to<EBPF::EBPFControlPSA>()->getHash(name);
        hash->processMethod(builder, method->method->name.name, method->expr, this);
        return;
    } else {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "%1%: Unexpected method call", method->expr);
    }
}

bool ControlBodyTranslatorPNA::preorder(const IR::AssignmentStatement *a) {
    if (auto methodCallExpr = a->right->to<IR::MethodCallExpression>()) {
        auto mi = P4::MethodInstance::resolve(methodCallExpr, control->program->refMap,
                                              control->program->typeMap);
        auto ext = mi->to<P4::ExternMethod>();
        if (ext == nullptr) {
            return false;
        }

        if (ext->originalExternType->name.name == "Hash") {
            cstring name = EBPF::EBPFObject::externalName(ext->object);
            auto hash = control->to<EBPF::EBPFControlPSA>()->getHash(name);
            // Before assigning a value to a left expression we have to calculate a hash.
            // Then the hash value is stored in a registerVar variable.
            hash->calculateHash(builder, ext->expr, this);
            builder->emitIndent();
        }
    }

    return EBPF::CodeGenInspector::preorder(a);
}
// =====================ActionTranslationVisitorPNA=============================
ActionTranslationVisitorPNA::ActionTranslationVisitorPNA(
    const EBPF::EBPFProgram *program, cstring valueName, const EBPF::EBPFTablePSA *table,
    const ConvertToBackendIR *tcIR, const IR::P4Action *act, bool isDefaultAction)
    : EBPF::CodeGenInspector(program->refMap, program->typeMap),
      EBPF::ActionTranslationVisitor(valueName, program),
      ControlBodyTranslatorPNA(program->as<EBPF::EBPFPipeline>().control, tcIR, table),
      table(table),
      isDefaultAction(isDefaultAction) {
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
        auto paramStr = Util::printf_format("p4tc_filter_fields.%s_%s_%s",
                                            table->table->container->name.originalName, actionName,
                                            expression->toString());
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
                ::error(ErrorType::ERR_UNSUPPORTED, "%1% field key must be at the end of whole key",
                        it->matchType);
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
                ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "Cannot emit a non-header type %1%",
                        expr);
            }

            cstring msgStr;
            builder->emitIndent();
            builder->append("if (");
            this->visit(expr);
            builder->append(".ebpf_valid) ");
            builder->blockStart();
            auto program = deparser->program;
            unsigned width = headerToEmit->width_bits();
            msgStr = Util::printf_format("Deparser: emitting header %s", expr->toString().c_str());
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
                    ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                            "Only headers with fixed widths supported %1%", f);
                    return;
                }
                bool noEndiannessConversion = false;
                auto annotations = f->getAnnotations()->annotations;
                for (auto anno : annotations) {
                    if (anno->name != ParseTCAnnotations::tcType) continue;
                    for (auto annoVal : anno->body) {
                        if (annoVal->text == "macaddr" || annoVal->text == "ipv4" ||
                            annoVal->text == "ipv6" || annoVal->text == "be16" ||
                            annoVal->text == "be32" || annoVal->text == "be64") {
                            noEndiannessConversion = true;
                            break;
                        }
                    }
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
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "Only headers with fixed widths supported %1%", hdrExpr);
        return;
    }
    unsigned widthToEmit = et->widthInBits();
    unsigned emitSize = 0;
    cstring swap = "", msgStr;

    if (widthToEmit <= 64) {
        if (program->options.emitTraceMessages) {
            builder->emitIndent();
            builder->blockStart();
            builder->emitIndent();
            builder->append("u64 tmp = ");
            visit(hdrExpr);
            builder->appendFormat(".%s", field.c_str());
            builder->endOfStatement(true);
            msgStr = Util::printf_format("Deparser: emitting field %s=0x%%llx (%u bits)", field,
                                         widthToEmit);
            builder->target->emitTraceMessage(builder, msgStr.c_str(), 1, "tmp");
            builder->blockEnd(true);
        }
    } else {
        msgStr = Util::printf_format("Deparser: emitting field %s (%u bits)", field, widthToEmit);
        builder->target->emitTraceMessage(builder, msgStr.c_str());
    }

    if (widthToEmit <= 8) {
        emitSize = 8;
    } else if (widthToEmit <= 16) {
        swap = "bpf_htons";
        emitSize = 16;
    } else if (widthToEmit <= 32) {
        swap = "htonl";
        emitSize = 32;
    } else if (widthToEmit <= 64) {
        swap = "htonll";
        emitSize = 64;
    }
    unsigned shift =
        widthToEmit < 8 ? (emitSize - alignment - widthToEmit) : (emitSize - widthToEmit);

    if (!swap.isNullOrEmpty() && !noEndiannessConversion) {
        builder->emitIndent();
        visit(hdrExpr);
        builder->appendFormat(".%s = %s(", field, swap);
        visit(hdrExpr);
        builder->appendFormat(".%s", field);
        if (shift != 0) builder->appendFormat(" << %d", shift);
        builder->append(")");
        builder->endOfStatement(true);
    }
    unsigned bitsInFirstByte = widthToEmit % 8;
    if (bitsInFirstByte == 0) bitsInFirstByte = 8;
    unsigned bitsInCurrentByte = bitsInFirstByte;
    unsigned left = widthToEmit;
    for (unsigned i = 0; i < (widthToEmit + 7) / 8; i++) {
        builder->emitIndent();
        builder->appendFormat("%s = ((char*)(&", program->byteVar.c_str());
        visit(hdrExpr);
        builder->appendFormat(".%s))[%d]", field, i);
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

void CRCChecksumAlgorithmPNA::emitUpdateMethod(EBPF::CodeBuilder *builder, int crcWidth) {
    // Note that this update method is optimized for our CRC16 and CRC32, custom
    // version may require other method of update. When data_size <= 64 bits,
    // applies host byte order for input data, otherwise network byte order is expected.
    if (crcWidth == 16) {
        // This function calculates CRC16 by definition, it is bit by bit. If input data has more
        // than 64 bit, the outer loop process bytes in network byte order - data pointer is
        // incremented. For data shorter than or equal 64 bits, bytes are processed in little endian
        // byte order - data pointer is decremented by outer loop in this case.
        // There is no need for lookup table.
        cstring code =
            "static __always_inline\n"
            "void crc16_update(u16 * reg, const u8 * data, "
            "u16 data_size, const u16 poly) {\n"
            "    if (data_size <= 8)\n"
            "        data += data_size - 1;\n"
            "    #pragma clang loop unroll(full)\n"
            "    for (u16 i = 0; i < data_size; i++) {\n"
            "        bpf_trace_message(\"CRC16: data byte: %x\\n\", *data);\n"
            "        *reg ^= *data;\n"
            "        for (u8 bit = 0; bit < 8; bit++) {\n"
            "            *reg = (*reg) & 1 ? ((*reg) >> 1) ^ poly : (*reg) >> 1;\n"
            "        }\n"
            "        if (data_size <= 8)\n"
            "            data--;\n"
            "        else\n"
            "            data++;\n"
            "    }\n"
            "}";
        builder->appendLine(code);
    } else if (crcWidth == 32) {
        // This function calculates CRC32 using two optimisations: slice-by-8 and Standard
        // Implementation. Both algorithms have to properly handle byte order depending on data
        // length. There are four cases which must be handled:
        // 1. Data size below 8 bytes - calculated using Standard Implementation in little endian
        //    byte order.
        // 2. Data size equal to 8 bytes - calculated using slice-by-8 in little endian byte order.
        // 3. Data size more than 8 bytes and multiply of 8 bytes - calculated using slice-by-8 in
        //    big endian byte order.
        // 4. Data size more than 8 bytes and not multiply of 8 bytes - calculated using slice-by-8
        //    and Standard Implementation both in big endian byte order.
        // Lookup table is necessary for both algorithms.
        cstring code =
            "static __always_inline\n"
            "void crc32_update(u32 * reg, const u8 * data, u16 data_size, const u32 poly) {\n"
            "    u32* current = (u32*) data;\n"
            "    u32 index = 0;\n"
            "    u32 lookup_key = 0;\n"
            "    u32 lookup_value = 0;\n"
            "    u32 lookup_value1 = 0;\n"
            "    u32 lookup_value2 = 0;\n"
            "    u32 lookup_value3 = 0;\n"
            "    u32 lookup_value4 = 0;\n"
            "    u32 lookup_value5 = 0;\n"
            "    u32 lookup_value6 = 0;\n"
            "    u32 lookup_value7 = 0;\n"
            "    u32 lookup_value8 = 0;\n"
            "    u16 tmp = 0;\n"
            "    if (crc32_table != NULL) {\n"
            "        for (u16 i = data_size; i >= 8; i -= 8) {\n"
            "            /* Vars one and two will have swapped byte order if data_size == 8 */\n"
            "            if (data_size == 8) current = (u32 *)(data + 4);\n"
            "            bpf_trace_message(\"CRC32: data dword: %x\\n\", *current);\n"
            "            u32 one = (data_size == 8 ? __builtin_bswap32(*current--) : *current++) ^ "
            "*reg;\n"
            "            bpf_trace_message(\"CRC32: data dword: %x\\n\", *current);\n"
            "            u32 two = (data_size == 8 ? __builtin_bswap32(*current--) : *current++);\n"
            "            lookup_key = (one & 0x000000FF);\n"
            "            lookup_value8 = crc32_table[(u16)(1792 + (u8)lookup_key)];\n"
            "            lookup_key = (one >> 8) & 0x000000FF;\n"
            "            lookup_value7 = crc32_table[(u16)(1536 + (u8)lookup_key)];\n"
            "            lookup_key = (one >> 16) & 0x000000FF;\n"
            "            lookup_value6 = crc32_table[(u16)(1280 + (u8)lookup_key)];\n"
            "            lookup_key = one >> 24;\n"
            "            lookup_value5 = crc32_table[(u16)(1024 + (u8)(lookup_key))];\n"
            "            lookup_key = (two & 0x000000FF);\n"
            "            lookup_value4 = crc32_table[(u16)(768 + (u8)lookup_key)];\n"
            "            lookup_key = (two >> 8) & 0x000000FF;\n"
            "            lookup_value3 = crc32_table[(u16)(512 + (u8)lookup_key)];\n"
            "            lookup_key = (two >> 16) & 0x000000FF;\n"
            "            lookup_value2 = crc32_table[(u16)(256 + (u8)lookup_key)];\n"
            "            lookup_key = two >> 24;\n"
            "            lookup_value1 = crc32_table[(u8)(lookup_key)];\n"
            "            *reg = lookup_value8 ^ lookup_value7 ^ lookup_value6 ^ lookup_value5 ^\n"
            "                   lookup_value4 ^ lookup_value3 ^ lookup_value2 ^ lookup_value1;\n"
            "            tmp += 8;\n"
            "        }\n"
            "        volatile int std_algo_lookup_key = 0;\n"
            "        if (data_size < 8) {\n"
            // Standard Implementation for little endian byte order
            "            unsigned char *currentChar = (unsigned char *) current;\n"
            "            currentChar += data_size - 1;\n"
            "            for (u16 i = tmp; i < data_size; i++) {\n"
            "                bpf_trace_message(\"CRC32: data byte: %x\\n\", *currentChar);\n"
            "                std_algo_lookup_key = (u32)(((*reg) & 0xFF) ^ *currentChar--);\n"
            "                if (std_algo_lookup_key >= 0) {\n"
            "                    lookup_value = "
            "crc32_table[(u8)(std_algo_lookup_key & 255)];\n"
            "                }\n"
            "                *reg = ((*reg) >> 8) ^ lookup_value;\n"
            "            }\n"
            "        } else {\n"
            // Standard Implementation for big endian byte order
            "            /* Consume data not processed by slice-by-8 algorithm above, "
            "these data are in network byte order */\n"
            "            unsigned char *currentChar = (unsigned char *) current;\n"
            "            for (u16 i = tmp; i < data_size; i++) {\n"
            "                bpf_trace_message(\"CRC32: data byte: %x\\n\", *currentChar);\n"
            "                std_algo_lookup_key = (u32)(((*reg) & 0xFF) ^ *currentChar++);\n"
            "                if (std_algo_lookup_key >= 0) {\n"
            "                    lookup_value = "
            "crc32_table[(u8)(std_algo_lookup_key & 255)];\n"
            "                }\n"
            "                *reg = ((*reg) >> 8) ^ lookup_value;\n"
            "            }\n"
            "        }\n"
            "    }\n"
            "}";
        builder->appendLine(code);
    }
}

// ===========================CRC16ChecksumAlgorithmPNA===========================

void CRC16ChecksumAlgorithmPNA::emitGlobals(EBPF::CodeBuilder *builder) {
    CRCChecksumAlgorithmPNA::emitUpdateMethod(builder, 16);

    cstring code =
        "static __always_inline "
        "u16 crc16_finalize(u16 reg) {\n"
        "    return reg;\n"
        "}";
    builder->appendLine(code);
}

// ===========================CRC32ChecksumAlgorithmPNA===========================

void CRC32ChecksumAlgorithmPNA::emitGlobals(EBPF::CodeBuilder *builder) {
    CRCChecksumAlgorithmPNA::emitUpdateMethod(builder, 32);

    cstring code =
        "static __always_inline "
        "u32 crc32_finalize(u32 reg) {\n"
        "    return reg ^ 0xFFFFFFFF;\n"
        "}";
    builder->appendLine(code);
}

}  // namespace TC
