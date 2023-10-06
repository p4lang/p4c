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
    builder->newline();
    cstring headerFile = getProgramName() + "_parser.h";
    builder->appendFormat("#include \"%s\"", headerFile);
    builder->endOfStatement(true);
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
    builder->appendLine(
        "struct internal_metadata {\n"
        "    __u16 pkt_ether_type;\n"
        "} __attribute__((aligned(4)));");
    builder->newline();
}

/* Generate headers and structs in p4 prog */
void PNAEbpfGenerator::emitTypes(EBPF::CodeBuilder *builder) const {
    PNAErrorCodesGen errorGen(builder);
    pipeline->program->apply(errorGen);

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

    // additional field to avoid compiler errors when both headers and user_metadata are empty.
    builder->emitIndent();
    builder->append("__u8 __hook");
    builder->endOfStatement(true);

    builder->blockEnd(false);
    builder->endOfStatement(true);
    builder->newline();
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
     * 4. BPF map definitions.
     * 5. XDP helper program.
     * 6. TC Pipeline program for post-parser.
     */

    // 1. Automatically generated comment.
    // Note we use inherited function from EBPFProgram.
    xdp->emitGeneratedComment(builder);

    /*
     * 2. Includes.
     */
    emitPNAIncludes(builder);

    /*
     * 3. Headers, structs
     */
    emitInternalStructures(builder);
    emitTypes(builder);
    emitGlobalHeadersMetadata(builder);

    /*
     * 4. BPF map definitions.
     */
    emitInstances(builder);

    /*
     * 5. XDP helper program.
     */
    xdp->emit(builder);

    /*
     * 6. TC Pipeline program for post-parser.
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
    emitPNAIncludes(builder);
    builder->newline();
    emitInstances(builder);
    pipeline->name = "tc-parse";
    pipeline->sectionName = "classifier/" + pipeline->name;
    pipeline->functionName = pipeline->name.replace("-", "_") + "_func";
    pipeline->emit(builder);
    builder->target->emitLicense(builder, pipeline->license);
}

void PNAArchTC::emitHeader(EBPF::CodeBuilder *builder) const {
    xdp->emitGeneratedComment(builder);
    builder->target->emitIncludes(builder);
    emitPreamble(builder);
    for (auto type : ebpfTypes) {
        type->emit(builder);
    }
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
        parser->headerType->to<EBPF::EBPFStructType>()->kind,
        parser->headerType->to<EBPF::EBPFStructType>()->name, parser->headers->name.name,
        compilerGlobalMetadata);

    builder->append(")");
    builder->newline();
    builder->blockStart();

    emitLocalVariables(builder);

    builder->newline();
    emitUserMetadataInstance(builder);

    emitCPUMAPHeadersInitializers(builder);
    if (name == "tc-parse") {
        builder->newline();
        emitCPUMAPInitializers(builder);
    } else {
        emitCPUMAPLookup(builder);
        builder->emitIndent();
        builder->append("if (!hdrMd)");
        builder->newline();
        builder->emitIndent();
        builder->emitIndent();
        builder->appendFormat("return %s;", dropReturnCode());
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
        builder->appendFormat("int i;");
        builder->newline();
        builder->emitIndent();
        builder->appendLine("#pragma clang loop unroll(disable)");
        builder->emitIndent();
        builder->appendFormat("for (i = 0; i < %d; i++) ", maxResubmitDepth);
        builder->blockStart();
        builder->emitIndent();
        builder->appendFormat("ret = %s(skb, ", func_name);

        builder->appendFormat("(%s %s *) %s, %s);",
                              parser->headerType->to<EBPF::EBPFStructType>()->kind,
                              parser->headerType->to<EBPF::EBPFStructType>()->name,
                              parser->headers->name.name, compilerGlobalMetadata);

        builder->newline();
        builder->appendFormat(
            "        if (%s->drop == 1) {\n"
            "            break;\n"
            "        }\n",
            compilerGlobalMetadata);
        builder->blockEnd(true);
        builder->newline();
        builder->emitIndent();
        builder->appendFormat("%s->recirculated = (i > 0);", compilerGlobalMetadata);
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

        emitHeaderInstances(builder);
        builder->newline();

        builder->emitIndent();
        builder->appendFormat("int ret = %d;", actUnspecCode);
        builder->newline();
        builder->emitIndent();
        builder->appendFormat("ret = %s(skb, ", func_name);

        builder->appendFormat("(%s %s *) %s, %s);",
                              parser->headerType->to<EBPF::EBPFStructType>()->kind,
                              parser->headerType->to<EBPF::EBPFStructType>()->name,
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
    builder->emitIndent();
    builder->appendFormat("unsigned %s = 0;", offsetVar.c_str());
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

//  This code is similar to compileExtractField function in PsaStateTranslationVisitor.
//  Handled TC "macaddr" annotation.
void PnaStateTranslationVisitor::compileExtractField(const IR::Expression *expr,
                                                     const IR::StructField *field,
                                                     unsigned alignment, EBPF::EBPFType *type) {
    auto width = dynamic_cast<EBPF::IHasWidth *>(type);
    if (width == nullptr) return;
    unsigned widthToExtract = width->widthInBits();
    auto program = state->parser->program;
    cstring msgStr;
    cstring fieldName = field->name.name;

    bool checkIfMAC = false;
    auto annolist = field->getAnnotations()->annotations;
    for (auto anno : annolist) {
        if (anno->name != ParseTCAnnotations::tcType) continue;
        auto annoBody = anno->body;
        for (auto annoVal : annoBody) {
            if (annoVal->text == "macaddr") {
                checkIfMAC = true;
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
        if (checkIfMAC) {
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

// =====================EBPFTablePNA=============================
void EBPFTablePNA::emitKeyType(EBPF::CodeBuilder *builder) {
    builder->emitIndent();
    builder->appendFormat("struct __attribute__((__packed__)) %s ", keyTypeName.c_str());
    builder->blockStart();

    EBPF::CodeGenInspector commentGen(program->refMap, program->typeMap);
    commentGen.setBuilder(builder);

    unsigned int structAlignment = 4;  // 4 by default

    builder->emitIndent();
    builder->appendLine("u32 keysz;");
    builder->emitIndent();
    builder->appendLine("u32 maskid;");

    if (keyGenerator != nullptr) {
        for (auto c : keyGenerator->keyElements) {
            auto mtdecl = program->refMap->getDeclaration(c->matchType->path, true);
            auto matchType = mtdecl->getNode()->to<IR::Declaration_ID>();

            if (!isMatchTypeSupported(matchType)) {
                ::error(ErrorType::ERR_UNSUPPORTED, "Match of type %1% not supported",
                        c->matchType);
            }

            auto ebpfType = ::get(keyTypes, c);
            cstring fieldName = ::get(keyFieldNames, c);

            if (ebpfType->is<EBPF::EBPFScalarType>() &&
                ebpfType->to<EBPF::EBPFScalarType>()->alignment() > structAlignment) {
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
        auto action = adecl->getNode()->to<IR::P4Action>();
        if (action->name.originalName == P4::P4CoreLibrary::instance().noAction.name) continue;
        cstring name = EBPF::EBPFObject::externalName(action);
        emitActionArguments(builder, action, name);
    }

    builder->blockEnd(false);
    builder->spc();
    builder->appendLine("u;");
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
    builder->emitIndent();
    builder->appendFormat("switch (%s->action) ", valueName.c_str());
    builder->blockStart();

    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->to<IR::P4Action>();
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
            unsigned width = dynamic_cast<EBPF::IHasWidth *>(etype)->widthInBits();

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

        builder->emitIndent();

        auto visitor = createActionTranslationVisitor(valueName, program);
        visitor->setBuilder(builder);
        visitor->copySubstitutions(codeGen);
        visitor->copyPointerVariables(codeGen);

        action->apply(*visitor);
        builder->newline();
        builder->emitIndent();
        builder->appendLine("break;");
        builder->decreaseIndent();
    }

    builder->emitIndent();
    builder->appendLine("default:");
    builder->increaseIndent();
    builder->target->emitTraceMessage(builder, "Control: Invalid action type, aborting");

    builder->emitIndent();
    builder->appendFormat("return %s", builder->target->abortReturnCode().c_str());
    builder->endOfStatement(true);
    builder->decreaseIndent();

    builder->blockEnd(true);

    if (!actionRunVariable.isNullOrEmpty()) {
        builder->emitIndent();
        builder->appendFormat("%s = %s->action", actionRunVariable.c_str(), valueName.c_str());
        builder->endOfStatement(true);
    }
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
    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->to<IR::P4Action>();
        // no need to define a constant for NoAction,
        // "case 0" will be explicitly generated in the action handling switch
        if (action->name.originalName == P4::P4CoreLibrary::instance().noAction.name) {
            continue;
        }
        unsigned int action_idx = tcIR->getActionId(tcIR->externalName(action));
        builder->emitIndent();
        builder->appendFormat("#define %s %d", p4ActionToActionIDName(action), action_idx);
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
    auto pipeline = dynamic_cast<const EBPF::EBPFPipeline *>(program);
    builder->appendFormat("if (%s->drop) ", pipeline->compilerGlobalMetadata);
    builder->blockStart();
    builder->target->emitTraceMessage(builder, "PreDeparser: dropping packet..");
    builder->emitIndent();
    builder->appendFormat("return %s;\n", builder->target->abortReturnCode().c_str());
    builder->blockEnd(true);
}

void IngressDeparserPNA::emit(EBPF::CodeBuilder *builder) {
    codeGen->setBuilder(builder);

    for (auto a : controlBlock->container->controlLocals) emitDeclaration(builder, a);

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
    auto pipeline = tlb->getMain()->to<IR::PackageBlock>();
    auto pipelineParser = pipeline->getParameterValue("main_parser");
    BUG_CHECK(pipelineParser != nullptr, "No parser block found");
    auto pipelineControl = pipeline->getParameterValue("main_control");
    BUG_CHECK(pipelineControl != nullptr, "No control block found");
    auto pipelineDeparser = pipeline->getParameterValue("main_deparser");
    BUG_CHECK(pipelineDeparser != nullptr, "No deparser block found");

    auto xdp = new EBPF::XDPHelpProgram(options);

    auto pipeline_converter = new ConvertToEbpfPipelineTC(
        "tc-ingress", EBPF::TC_INGRESS, options, pipelineParser->to<IR::ParserBlock>(),
        pipelineControl->to<IR::ControlBlock>(), pipelineDeparser->to<IR::ControlBlock>(), refmap,
        typemap, tcIR);
    pipeline->apply(*pipeline_converter);
    tlb->getProgram()->apply(*pipeline_converter);
    auto tcIngress = pipeline_converter->getEbpfPipeline();

    return new PNAArchTC(options, ebpfTypes, xdp, tcIngress);
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
    program->to<EBPF::EBPFPipeline>()->control = control;
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
        if (b->is<IR::Block>()) {
            this->visit(b->to<IR::Block>());
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

cstring ControlBodyTranslatorPNA::getParamName(const IR::PathExpression *expr) {
    return expr->path->name.name;
}

bool ControlBodyTranslatorPNA::checkPnaPortMem(const IR::Member *m) {
    if (m->expr != nullptr && m->expr->type != nullptr) {
        if (auto str_type = m->expr->type->to<IR::Type_Struct>()) {
            if (str_type->name == "pna_main_input_metadata_t" && m->member.name == "input_port") {
                return true;
            }
        }
    }
    return false;
}

void ControlBodyTranslatorPNA::processFunction(const P4::ExternFunction *function) {
    if (function->expr->toString() == "send_to_port" ||
        function->expr->toString() == "drop_packet") {
        if (function->expr->toString() != "drop_packet") {
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
            if (a->expression->is<IR::Member>() &&
                checkPnaPortMem(a->expression->to<IR::Member>())) {
                builder->append("skb->ifindex");
            } else {
                visit(a);
            }
        }
        builder->append(")");
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
        builder->appendLine("    .pipeid = 1,");
        builder->emitIndent();
        auto tblId = tcIR->getTableId(method->object->getName().originalName);
        BUG_CHECK(tblId != 0, "Table ID not found");
        builder->appendFormat("    .tblid = %d", tblId);
        builder->newline();
        builder->emitIndent();
        builder->appendLine("};");
        builder->emitIndent();
        builder->appendFormat("struct %s %s = {}", table->keyTypeName.c_str(), keyname.c_str());
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
            if (ebpfType->is<EBPF::EBPFScalarType>()) {
                scalar = ebpfType->to<EBPF::EBPFScalarType>();
                unsigned width = scalar->implementationWidthInBits();
                memcpy = !EBPF::EBPFScalarType::generatesScalar(width);

                if (width <= 8) {
                    swap = "";  // single byte, nothing to swap
                } else if (width <= 16) {
                    swap = "bpf_htons";
                } else if (width <= 32) {
                    swap = "bpf_htonl";
                } else if (width <= 64) {
                    swap = "bpf_htonll";
                } else {
                    // The code works with fields wider than 64 bits for PSA architecture. It is
                    // shared with filter model, so should work but has not been tested. Error
                    // message is preserved for filter model because existing tests expect it.
                    // TODO: handle width > 64 bits for filter model
                    if (table->program->options.arch.isNullOrEmpty() ||
                        table->program->options.arch == "filter") {
                        ::error(ErrorType::ERR_UNSUPPORTED,
                                "%1%: fields wider than 64 bits are not supported yet", fieldName);
                    }
                }
            }

            bool isLPMKeyBigEndian = false;
            if (table->isLPMTable()) {
                if (c->matchType->path->name.name == P4::P4CoreLibrary::instance().lpmMatch.name)
                    isLPMKeyBigEndian = true;
            }

            builder->emitIndent();
            if (memcpy) {
                builder->appendFormat("__builtin_memcpy(&(%s.%s[0]), &(", keyname.c_str(),
                                      fieldName.c_str());
                table->codeGen->visit(c->expression);
                builder->appendFormat("[0]), %d)", scalar->bytesRequired());
            } else {
                builder->appendFormat("%s.%s = ", keyname.c_str(), fieldName.c_str());
                if (isLPMKeyBigEndian) builder->appendFormat("%s(", swap.c_str());
                table->codeGen->visit(c->expression);
                if (isLPMKeyBigEndian) builder->append(")");
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
        builder->appendLine("act_bpf = bpf_skb_p4tc_tbl_read(skb, &params, &key, sizeof(key));");
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
    builder->appendFormat("%s = 1", control->hitVariable.c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);

    if (table->cacheEnabled()) {
        table->emitCacheUpdate(builder, keyname, valueName);
        builder->blockEnd(true);
    }

    builder->emitIndent();
    builder->appendFormat("if (%s != NULL) ", valueName.c_str());
    builder->blockStart();
    builder->emitIndent();
    builder->appendLine("/* run action */");
    table->emitAction(builder, valueName, actionVariableName);
    toDereference.clear();

    builder->blockEnd(false);
    builder->appendFormat(" else ");
    builder->blockStart();
    if (table->dropOnNoMatchingEntryFound()) {
        builder->target->emitTraceMessage(builder, "Control: Entry not found, aborting");
        builder->emitIndent();
        builder->appendFormat("return %s", builder->target->abortReturnCode().c_str());
        builder->endOfStatement(true);
    } else {
        builder->target->emitTraceMessage(builder,
                                          "Control: Entry not found, executing implicit NoAction");
    }
    builder->endOfStatement(true);
    builder->blockEnd(true);
    builder->blockEnd(true);

    msgStr = Util::printf_format("Control: %s applied", method->object->getName().name);
    builder->target->emitTraceMessage(builder, msgStr.c_str());
}

// =====================ActionTranslationVisitorPNA=============================
ActionTranslationVisitorPNA::ActionTranslationVisitorPNA(const EBPF::EBPFProgram *program,
                                                         cstring valueName,
                                                         const EBPF::EBPFTablePSA *table)
    : EBPF::CodeGenInspector(program->refMap, program->typeMap),
      EBPF::ActionTranslationVisitor(valueName, program),
      ControlBodyTranslatorPNA(program->to<EBPF::EBPFPipeline>()->control),
      table(table) {}

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

    return EBPF::ActionTranslationVisitor::getParamInstanceName(expression);
}

cstring ActionTranslationVisitorPNA::getParamName(const IR::PathExpression *expr) {
    if (isActionParameter(expr)) {
        return getParamInstanceName(expr);
    }
    return ControlBodyTranslatorPNA::getParamName(expr);
}

EBPF::ActionTranslationVisitor *EBPFTablePNA::createActionTranslationVisitor(
    cstring valueName, const EBPF::EBPFProgram *program) const {
    return new ActionTranslationVisitorPNA(program->to<EBPF::EBPFPipeline>(), valueName, this);
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
                auto et = dynamic_cast<EBPF::IHasWidth *>(etype);
                if (et == nullptr) {
                    ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                            "Only headers with fixed widths supported %1%", f);
                    return;
                }
                bool checkIfMAC = false;
                auto annolist = f->getAnnotations()->annotations;
                for (auto anno : annolist) {
                    if (anno->name != ParseTCAnnotations::tcType) continue;
                    auto annoBody = anno->body;
                    for (auto annoVal : annoBody) {
                        if (annoVal->text == "macaddr") {
                            checkIfMAC = true;
                            break;
                        }
                    }
                }
                emitField(builder, f->name, expr, alignment, etype, checkIfMAC);
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
                                             EBPF::EBPFType *type, bool isMAC) {
    auto program = deparser->program;

    auto et = dynamic_cast<EBPF::IHasWidth *>(type);
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

    if (!swap.isNullOrEmpty() && !isMAC) {
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

}  // namespace TC
