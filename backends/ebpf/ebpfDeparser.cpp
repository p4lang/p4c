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

#include "ebpfDeparser.h"

namespace EBPF {

DeparserBodyTranslator::DeparserBodyTranslator(const EBPFDeparser *deparser) :
        CodeGenInspector(deparser->program->refMap, deparser->program->typeMap),
        ControlBodyTranslator(deparser), deparser(deparser) {
    setName("DeparserBodyTranslator");
}

bool DeparserBodyTranslator::preorder(const IR::MethodCallExpression* expression) {
    auto mi = P4::MethodInstance::resolve(expression,
                                          control->program->refMap,
                                          control->program->typeMap);
    auto ext = mi->to<P4::ExternMethod>();
    if (ext != nullptr) {
        // We skip headers emit processing which is handled by DeparserHdrEmitTranslator
        if (ext->method->name.name == p4lib.packetOut.emit.name)
            return false;
    }

    return ControlBodyTranslator::preorder(expression);
}

DeparserPrepareBufferTranslator::DeparserPrepareBufferTranslator(const EBPFDeparser *deparser) :
        CodeGenInspector(deparser->program->refMap, deparser->program->typeMap),
        ControlBodyTranslator(deparser), deparser(deparser) {
    setName("DeparserPrepareBufferTranslator");
}

bool DeparserPrepareBufferTranslator::preorder(const IR::BlockStatement* s) {
    for (auto a : s->components) {
        if (a->is<IR::MethodCallStatement>()) {
            visit(a);
        }
    }

    return false;
}

bool DeparserPrepareBufferTranslator::preorder(const IR::MethodCallExpression* expression) {
    auto mi = P4::MethodInstance::resolve(expression,
                                          control->program->refMap,
                                          control->program->typeMap);
    auto ext = mi->to<P4::ExternMethod>();
    if (ext != nullptr) {
        processMethod(ext);
        return false;
    }

    return false;
}

void DeparserPrepareBufferTranslator::processMethod(const P4::ExternMethod *method) {
    if (method->method->name.name == p4lib.packetOut.emit.name) {
        auto decl = method->object;
        if (decl == deparser->packet_out) {
            if (method->expr->arguments->size() != 1) {
                ::error(ErrorType::ERR_MODEL,
                        "Not enough arguments to emit() method, exactly 1 required");
            }

            auto expr = method->expr->arguments->at(0)->expression;
            auto exprType = deparser->program->typeMap->getType(expr);
            auto headerToEmit = exprType->to<IR::Type_Header>();
            if (headerToEmit == nullptr) {
                ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                        "Cannot emit a non-header type %1%", expr);
                return;
            }

            unsigned width = headerToEmit->width_bits();
            builder->emitIndent();
            builder->append("if (");
            this->visit(expr);
            builder->append(".ebpf_valid) ");
            builder->blockStart();
            builder->emitIndent();
            builder->appendFormat("%s += %d;",
                                  this->deparser->outerHdrLengthVar.c_str(), width);
            builder->newline();
            builder->blockEnd(true);
        }
    }
}

DeparserHdrEmitTranslator::DeparserHdrEmitTranslator(const EBPFDeparser *deparser) :
        CodeGenInspector(deparser->program->refMap, deparser->program->typeMap),
        DeparserPrepareBufferTranslator(deparser), deparser(deparser) {
    setName("DeparserHdrEmitTranslator");
}

void DeparserHdrEmitTranslator::processMethod(const P4::ExternMethod *method) {
    // This method handles packet_out.emit() only and is intended to skip other externs
    if (method->method->name.name == p4lib.packetOut.emit.name) {
        auto decl = method->object;
        if (decl == deparser->packet_out) {
            auto expr = method->expr->arguments->at(0)->expression;
            auto exprType = deparser->program->typeMap->getType(expr);
            auto headerToEmit = exprType->to<IR::Type_Header>();
            if (headerToEmit == nullptr) {
                ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
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
            msgStr = Util::printf_format("Deparser: emitting header %s",
                                         expr->toString().c_str());
            builder->target->emitTraceMessage(builder, msgStr.c_str());

            builder->emitIndent();
            builder->appendFormat("if (%s < %s + BYTES(%s + %d)) ",
                                  program->packetEndVar.c_str(),
                                  program->packetStartVar.c_str(),
                                  program->offsetVar.c_str(), width);
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
                auto etype = EBPFTypeFactory::instance->create(ftype);
                auto et = dynamic_cast<EBPF::IHasWidth *>(etype);
                if (et == nullptr) {
                    ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                            "Only headers with fixed widths supported %1%", f);
                    return;
                }
                emitField(builder, f->name, expr, alignment, etype);
                alignment += et->widthInBits();
                alignment %= 8;
            }
            builder->blockEnd(true);
        } else {
            BUG("emit() should only be invoked for packet_out");
        }
    }
}

void DeparserHdrEmitTranslator::emitField(CodeBuilder* builder, cstring field,
                                          const IR::Expression* hdrExpr, unsigned int alignment,
                                          EBPF::EBPFType* type) {
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
            msgStr = Util::printf_format("Deparser: emitting field %s=0x%%llx (%u bits)",
                                         field, widthToEmit);
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
    unsigned bytes = ROUNDUP(widthToEmit, 8);
    unsigned shift = widthToEmit < 8 ?
                     (emitSize - alignment - widthToEmit) : (emitSize - widthToEmit);

    if (!swap.isNullOrEmpty()) {
        builder->emitIndent();
        visit(hdrExpr);
        builder->appendFormat(".%s = %s(", field, swap);
        visit(hdrExpr);
        builder->appendFormat(".%s", field);
        if (shift != 0)
            builder->appendFormat(" << %d", shift);
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
        unsigned bitsToWrite =
                bitsInCurrentByte > freeBits ? freeBits : bitsInCurrentByte;
        BUG_CHECK((bitsToWrite > 0) && (bitsToWrite <= 8),
                  "invalid bitsToWrite %d", bitsToWrite);
        builder->emitIndent();
        if (alignment == 0 && bitsToWrite == 8) {  // write whole byte
            builder->appendFormat(
                    "write_byte(%s, BYTES(%s) + %d, (%s))",
                    program->packetStartVar.c_str(),
                    program->offsetVar.c_str(),
                    widthToEmit > 64 ? bytes - i - 1 : i,  // reversed order for wider fields
                    program->byteVar.c_str());
        } else {  // write partial
            shift = (8 - alignment - bitsToWrite);
            builder->appendFormat(
                    "write_partial(%s + BYTES(%s) + %d, %d, %d, (%s >> %d))",
                    program->packetStartVar.c_str(),
                    program->offsetVar.c_str(),
                    widthToEmit > 64 ? bytes - i - 1 : i,  // reversed order for wider fields
                    bitsToWrite,
                    shift,
                    program->byteVar.c_str(),
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
                builder->appendFormat(
                        "write_byte(%s, BYTES(%s) + %d + 1, (%s << %d))",
                        program->packetStartVar.c_str(),
                        program->offsetVar.c_str(),
                        widthToEmit > 64 ? bytes - i - 1 : i,  // reversed order for wider fields
                        program->byteVar.c_str(),
                        8 - alignment % 8);
            } else {
                builder->appendFormat(
                        "write_partial(%s + BYTES(%s) + %d + 1, %d, %d, (%s))",
                        program->packetStartVar.c_str(),
                        program->offsetVar.c_str(),
                        widthToEmit > 64 ? bytes - i - 1 : i,  // reversed order for wider fields
                        bitsToWrite,
                        8 + alignment - bitsToWrite,
                        program->byteVar.c_str());
            }
            builder->endOfStatement(true);
            left -= bitsToWrite;
        }
        alignment = (alignment + bitsToWrite) % 8;
    }
    builder->emitIndent();
    builder->appendFormat("%s += %d", program->offsetVar.c_str(),
                          widthToEmit);
    builder->endOfStatement(true);
    builder->newline();
}

void EBPFDeparser::emitBufferAdjusts(CodeBuilder *builder) const {
    builder->newline();
    builder->emitIndent();

    cstring offsetVar = program->offsetVar;
    builder->appendFormat("int %s = BYTES(%s) - BYTES(%s)",
                          outerHdrOffsetVar.c_str(),
                          outerHdrLengthVar.c_str(),
                          offsetVar.c_str());
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("if (%s != 0) ", outerHdrOffsetVar.c_str());
    builder->blockStart();
    builder->target->emitTraceMessage(builder, "Deparser: pkt_len adjusting by %d B",
                                      1, outerHdrOffsetVar.c_str());
    builder->emitIndent();
    builder->appendFormat("int %s = 0", returnCode.c_str());
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("%s = ", returnCode.c_str());
    builder->target->emitResizeBuffer(builder, program->model.CPacketName.str(),
                                      outerHdrOffsetVar);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("if (%s) ", returnCode.c_str());
    builder->blockStart();
    builder->target->emitTraceMessage(builder, "Deparser: pkt_len adjust failed");
    builder->emitIndent();
    // We immediately return instead of jumping to reject state.
    // It avoids reaching BPF_COMPLEXITY_LIMIT_JMP_SEQ.
    builder->appendFormat("return %s;", builder->target->abortReturnCode().c_str());
    builder->newline();
    builder->blockEnd(true);
    builder->target->emitTraceMessage(builder, "Deparser: pkt_len adjusted");
    builder->blockEnd(true);
}

void EBPFDeparser::emit(CodeBuilder* builder) {
    codeGen->setBuilder(builder);

    for (auto a : controlBlock->container->controlLocals)
        emitDeclaration(builder, a);

    emitDeparserExternCalls(builder);
    builder->newline();

    emitPreDeparser(builder);

    builder->emitIndent();
    builder->appendFormat("int %s = 0", this->outerHdrLengthVar.c_str());
    builder->endOfStatement(true);

    auto prepareBufferTranslator = new DeparserPrepareBufferTranslator(this);
    prepareBufferTranslator->setBuilder(builder);
    prepareBufferTranslator->copyPointerVariables(codeGen);
    prepareBufferTranslator->substitute(this->headers, this->parserHeaders);
    controlBlock->container->body->apply(*prepareBufferTranslator);

    emitBufferAdjusts(builder);

    builder->emitIndent();
    builder->appendFormat("%s = %s;",
                          program->packetStartVar,
                          builder->target->dataOffset(program->model.CPacketName.str()));
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("%s = %s;",
                          program->packetEndVar,
                          builder->target->dataEnd(program->model.CPacketName.str()));
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("%s = 0", program->offsetVar.c_str());
    builder->endOfStatement(true);

    // emit headers
    auto hdrEmitTranslator = new DeparserHdrEmitTranslator(this);
    hdrEmitTranslator->setBuilder(builder);
    hdrEmitTranslator->copyPointerVariables(codeGen);
    hdrEmitTranslator->substitute(this->headers, this->parserHeaders);
    controlBlock->container->body->apply(*hdrEmitTranslator);

    builder->newline();
}

}  // namespace EBPF
