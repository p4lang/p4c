/*
Copyright (C) 2024 Intel Corporation

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

#include "tcExterns.h"

namespace P4::TC {

void EBPFRegisterPNA::emitInitializer(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                                      ControlBodyTranslatorPNA *translator) {
    auto externName = method->originalExternType->name.name;
    auto index = method->expr->arguments->at(0)->expression;
    builder->emitIndent();
    builder->appendLine("__builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));");
    builder->emitIndent();
    builder->appendLine("ext_params.pipe_id = p4tc_filter_fields.pipeid;");
    builder->emitIndent();
    auto extId = translator->tcIR->getExternId(externName);
    BUG_CHECK(!extId.isNullOrEmpty(), "Extern ID not found");
    builder->appendFormat("ext_params.ext_id = %s;", extId);
    builder->newline();
    builder->emitIndent();
    auto instId = translator->tcIR->getExternInstanceId(externName, this->instanceName);
    BUG_CHECK(instId != 0, "Extern instance ID not found");
    builder->appendFormat("ext_params.inst_id = %d;", instId);
    builder->newline();
    builder->emitIndent();
    builder->append("ext_params.index = ");
    translator->visit(index);
    builder->endOfStatement(true);
}

void EBPFRegisterPNA::emitRegisterRead(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                                       ControlBodyTranslatorPNA *translator,
                                       const IR::Expression *leftExpression) {
    emitInitializer(builder, method, translator);
    builder->newline();
    builder->emitIndent();
    builder->appendLine(
        "ext_val_ptr = bpf_p4tc_extern_md_read(skb, &ext_params, sizeof(ext_params));");
    builder->emitIndent();
    builder->appendLine("if (!ext_val_ptr) ");
    builder->emitIndent();
    builder->appendFormat("     return %s", builder->target->abortReturnCode().c_str());
    builder->endOfStatement(true);
    builder->appendLine("ext_val = *ext_val_ptr;");
    if (leftExpression != nullptr) {
        builder->emitIndent();
        builder->append("__builtin_memcpy/*C*/(&");
        translator->visit(leftExpression);
        builder->append(", ext_val.out_params, sizeof(");
        this->valueType->declare(builder, cstring::empty, false);
        builder->append("));");
    }
}

void EBPFRegisterPNA::emitRegisterWrite(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                                        ControlBodyTranslatorPNA *translator) {
    auto value = method->expr->arguments->at(1)->expression;
    emitInitializer(builder, method, translator);

    builder->newline();
    builder->emitIndent();
    builder->append("__builtin_memcpy/*D*/(ext_val.out_params, &");
    translator->visit(value);
    builder->append(", sizeof(");
    this->valueType->declare(builder, cstring::empty, false);
    builder->append("));");
    builder->newline();
    builder->emitIndent();
    builder->appendLine(
        "bpf_p4tc_extern_md_write(skb, &ext_params, sizeof(ext_params), &ext_val, "
        "sizeof(ext_val));");
}

void EBPFCounterPNA::emitDirectMethodInvocation(EBPF::CodeBuilder *builder,
                                                const P4::ExternMethod *method,
                                                const ConvertToBackendIR *tcIR) {
    if (method->method->name.name != "count") {
        ::P4::error(ErrorType::ERR_UNSUPPORTED, "Unexpected method %1%", method->expr);
        return;
    }
    BUG_CHECK(isDirect, "Bad Counter invocation");
    BUG_CHECK(method->expr->arguments->size() == 0, "Expected no arguments for %1%", method->expr);
    emitCounterUpdate(builder, tcIR);
}

void EBPFCounterPNA::emitCounterUpdate(EBPF::CodeBuilder *builder, const ConvertToBackendIR *tcIR) {
    builder->emitIndent();
    builder->appendLine("__builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));");
    builder->emitIndent();
    builder->appendLine("ext_params.pipe_id = p4tc_filter_fields.pipeid;");
    auto tblId = tcIR->getTableId(tblname);
    BUG_CHECK(tblId != 0, "Table ID not found");
    builder->emitIndent();
    builder->appendFormat("ext_params.tbl_id = %d;", tblId);
    builder->newline();
    builder->emitIndent();
    builder->appendLine("ext_params.flags = P4TC_EXT_CNT_DIRECT;");
    builder->emitIndent();

    if (type == CounterType::BYTES) {
        builder->append(
            "bpf_p4tc_extern_count_bytes(skb, &ext_params, sizeof(ext_params), &key, "
            "sizeof(key))");
    } else if (type == CounterType::PACKETS) {
        builder->append(
            "bpf_p4tc_extern_count_pkts(skb, &ext_params, sizeof(ext_params), &key, "
            "sizeof(key))");
    } else if (type == CounterType::PACKETS_AND_BYTES) {
        builder->append(
            "bpf_p4tc_extern_count_pktsnbytes(skb, &ext_params, sizeof(ext_params), &key, "
            "sizeof(key))");
    }
}

void EBPFCounterPNA::emitMethodInvocation(EBPF::CodeBuilder *builder,
                                          const P4::ExternMethod *method,
                                          ControlBodyTranslatorPNA *translator) {
    if (method->method->name.name != "count") {
        ::P4::error(ErrorType::ERR_UNSUPPORTED, "Unexpected method %1%", method->expr);
        return;
    }
    BUG_CHECK(!isDirect, "DirectCounter used outside of table");
    BUG_CHECK(method->expr->arguments->size() == 1, "Expected just 1 argument for %1%",
              method->expr);
    emitCount(builder, method, translator);
}

void EBPFCounterPNA::emitCount(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                               ControlBodyTranslatorPNA *translator) {
    builder->emitIndent();
    builder->appendLine("__builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));");
    builder->emitIndent();
    builder->appendLine("ext_params.pipe_id = p4tc_filter_fields.pipeid;");
    auto externName = method->originalExternType->name.name;
    auto instanceName = di->toString();
    auto index = method->expr->arguments->at(0)->expression;
    builder->emitIndent();
    auto extId = translator->tcIR->getExternId(externName);
    BUG_CHECK(!extId.isNullOrEmpty(), "Extern ID not found");
    builder->appendFormat("ext_params.ext_id = %s;", extId);
    builder->newline();
    builder->emitIndent();
    auto instId = translator->tcIR->getExternInstanceId(externName, instanceName);
    BUG_CHECK(instId != 0, "Extern instance ID not found");
    builder->appendFormat("ext_params.inst_id = %d;", instId);
    builder->newline();
    builder->emitIndent();
    builder->append("ext_params.index = ");
    translator->visit(index);
    builder->endOfStatement(true);
    builder->newline();
    builder->emitIndent();
    builder->appendLine("ext_params.flags = P4TC_EXT_CNT_INDIRECT;");
    builder->emitIndent();

    if (type == CounterType::BYTES) {
        builder->append(
            "bpf_p4tc_extern_count_bytes(skb, &ext_params, sizeof(ext_params), NULL, 0)");
    } else if (type == CounterType::PACKETS) {
        builder->append(
            "bpf_p4tc_extern_count_pkts(skb, &ext_params, sizeof(ext_params), NULL, 0)");
    } else if (type == CounterType::PACKETS_AND_BYTES) {
        builder->append(
            "bpf_p4tc_extern_count_pktsnbytes(skb, &ext_params, sizeof(ext_params), NULL, 0)");
    }
}

void EBPFChecksumPNA::init(const EBPF::EBPFProgram *program, cstring name, int type) {
    engine = EBPFHashAlgorithmTypeFactoryPNA::instance()->create(type, program, name);

    if (engine == nullptr) {
        if (declaration->arguments->empty())
            ::P4::error(ErrorType::ERR_UNSUPPORTED, "InternetChecksum not yet implemented");
        else
            ::P4::error(ErrorType::ERR_UNSUPPORTED, "Hash algorithm not yet implemented: %1%",
                        declaration->arguments->at(0));
    }
}

void EBPFInternetChecksumPNA::processMethod(EBPF::CodeBuilder *builder, cstring method,
                                            const IR::MethodCallExpression *expr,
                                            Visitor *visitor) {
    engine->setVisitor(visitor);
    if (method == "add") {
        engine->emitAddData(builder, 0, expr);
    } else if (method == "subtract") {
        engine->emitSubtractData(builder, 0, expr);
    } else if (method == "get_state") {
        engine->emitGetInternalState(builder);
    } else if (method == "set_state") {
        engine->emitSetInternalState(builder, expr);
    } else if (method == "clear") {
        engine->emitClear(builder);
    } else if (method == "get") {
        engine->emitGet(builder);
    } else {
        ::P4::error(ErrorType::ERR_UNEXPECTED, "Unexpected method call %1%", expr);
    }
}

void InternetChecksumAlgorithmPNA::emitVariables(EBPF::CodeBuilder *builder,
                                                 const IR::Declaration_Instance *decl) {
    (void)decl;
    stateVar = program->refMap->newName(baseName + "_state");
    csumVar = program->refMap->newName(baseName + "_csum");
    builder->emitIndent();
    builder->appendFormat("u16 %s = 0", stateVar.c_str());
    builder->endOfStatement(true);
    builder->appendFormat("struct p4tc_ext_csum_params %s = {};", csumVar.c_str());
    builder->newline();
}

void InternetChecksumAlgorithmPNA::emitClear(EBPF::CodeBuilder *builder) {
    builder->emitIndent();
    builder->appendFormat("bpf_p4tc_ext_csum_16bit_complement_clear(&%s, sizeof(%s));",
                          csumVar.c_str(), csumVar.c_str());
    builder->newline();
}

void InternetChecksumAlgorithmPNA::emitAddData(EBPF::CodeBuilder *builder,
                                               const ArgumentsList &arguments) {
    updateChecksum(builder, arguments, true);
}

void InternetChecksumAlgorithmPNA::emitGet(EBPF::CodeBuilder *builder) {
    builder->appendFormat("(u16) bpf_p4tc_ext_csum_16bit_complement_get(&%s, sizeof(%s));",
                          csumVar.c_str(), csumVar.c_str());
    builder->newline();
}

void InternetChecksumAlgorithmPNA::emitSubtractData(EBPF::CodeBuilder *builder,
                                                    const ArgumentsList &arguments) {
    updateChecksum(builder, arguments, false);
}

void InternetChecksumAlgorithmPNA::emitGetInternalState(EBPF::CodeBuilder *builder) {
    builder->append(stateVar);
}

void InternetChecksumAlgorithmPNA::emitSetInternalState(EBPF::CodeBuilder *builder,
                                                        const IR::MethodCallExpression *expr) {
    if (expr->arguments->size() != 1) {
        ::P4::error(ErrorType::ERR_UNEXPECTED, "Expected exactly 1 argument %1%", expr);
        return;
    }
    builder->emitIndent();
    builder->appendFormat("%s = ", stateVar.c_str());
    visitor->visit(expr->arguments->at(0)->expression);
    builder->endOfStatement(true);
    builder->emitIndent();
    builder->appendFormat("bpf_p4tc_ext_csum_16bit_complement_set_state(&%s, sizeof(%s), ",
                          csumVar.c_str(), csumVar.c_str());
    visitor->visit(expr->arguments->at(0)->expression);
    builder->appendLine(");");
}

void InternetChecksumAlgorithmPNA::updateChecksum(EBPF::CodeBuilder *builder,
                                                  const ArgumentsList &arguments, bool addData) {
    cstring tmpVar = program->refMap->newName(baseName + "_tmp");

    builder->emitIndent();
    builder->blockStart();

    builder->emitIndent();
    builder->appendFormat("u16 %s = 0", tmpVar.c_str());
    builder->endOfStatement(true);

    int remainingBits = 16, bitsToRead;
    for (auto field : arguments) {
        auto fieldType = field->type->to<IR::Type_Bits>();
        if (fieldType == nullptr) {
            ::P4::error(ErrorType::ERR_UNSUPPORTED, "Unsupported field type: %1%", field->type);
            return;
        }
        const int width = fieldType->width_bits();
        bitsToRead = width;

        cstring field_temp = "_temp"_cs;
        cstring fieldByteOrder = "HOST"_cs;
        auto tcTarget = dynamic_cast<const EBPF::P4TCTarget *>(builder->target);
        fieldByteOrder = tcTarget->getByteOrder(program->typeMap, NULL, field);
        if (fieldByteOrder == "NETWORK"_cs) {
            const char *strToken = field->toString().findlast('.');
            if (strToken != nullptr) {
                cstring extract(strToken + 1);
                field_temp = program->refMap->newName(extract + "_temp");
            }
        }
        if (width > 64) {
            if (remainingBits != 16) {
                ::P4::error(
                    ErrorType::ERR_UNSUPPORTED,
                    "%1%: field wider than 64 bits must be aligned to 16 bits in input data",
                    field);
                continue;
            }
            if (width % 16 != 0) {
                ::P4::error(
                    ErrorType::ERR_UNSUPPORTED,
                    "%1%: field wider than 64 bits must have size in bits multiply of 16 bits",
                    field);
                continue;
            }

            // Let's convert internal array into an array of u16 and calc csum for such entries.
            // Byte order conversion is required, because csum is calculated in host byte order
            // but data is preserved in network byte order.
            const unsigned arrayEntries = width / 16;
            for (unsigned i = 0; i < arrayEntries; ++i) {
                builder->emitIndent();
                builder->appendFormat("%s = htons(((u16 *)(", tmpVar.c_str());
                visitor->visit(field);
                builder->appendFormat("))[%u])", i);
                builder->endOfStatement(true);

                // update checksum
                builder->target->emitTraceMessage(builder, "InternetChecksum: word=0x%llx", 1,
                                                  tmpVar.c_str());
                builder->emitIndent();
                if (addData) {
                    builder->appendFormat(
                        "bpf_p4tc_ext_csum_16bit_complement_add(&%s, sizeof(%s), "
                        "&%s, sizeof(%s))",
                        csumVar.c_str(), csumVar.c_str(), tmpVar.c_str(), tmpVar.c_str());
                } else {
                    builder->appendFormat(
                        "bpf_p4tc_ext_csum_16bit_complement_sub(&%s, sizeof(%s), "
                        "&%s, sizeof(%s))",
                        csumVar.c_str(), csumVar.c_str(), tmpVar.c_str(), tmpVar.c_str());
                }
                builder->endOfStatement(true);
            }
        } else {  // fields smaller or equal than 64 bits
            while (bitsToRead > 0) {
                if (fieldByteOrder == "NETWORK"_cs && bitsToRead == width) {
                    auto etype = EBPF::EBPFTypeFactory::instance->create(field->type);
                    builder->emitIndent();
                    etype->declare(builder, field_temp, false);
                    builder->appendFormat(" = %s(", getConvertByteOrderFunction(width, "HOST"_cs));
                    visitor->visit(field);
                    builder->appendLine(");");
                }
                if (remainingBits == 16) {
                    builder->emitIndent();
                    builder->appendFormat("%s = ", tmpVar.c_str());
                } else {
                    builder->append(" | ");
                }

                // TODO: add masks for fields, however they should not exceed declared width.
                if (bitsToRead < remainingBits) {
                    remainingBits -= bitsToRead;
                    builder->append("(");
                    if (fieldByteOrder == "NETWORK"_cs) {
                        builder->appendFormat("%s", field_temp);
                    } else {
                        visitor->visit(field);
                    }
                    builder->appendFormat(" << %d)", remainingBits);
                    bitsToRead = 0;
                } else if (bitsToRead == remainingBits) {
                    remainingBits = 0;
                    if (fieldByteOrder == "NETWORK"_cs) {
                        builder->appendFormat("%s", field_temp);
                    } else {
                        visitor->visit(field);
                    }
                    bitsToRead = 0;
                } else if (bitsToRead > remainingBits) {
                    bitsToRead -= remainingBits;
                    remainingBits = 0;
                    builder->append("(");
                    if (fieldByteOrder == "NETWORK"_cs) {
                        builder->appendFormat("%s", field_temp);
                    } else {
                        visitor->visit(field);
                    }
                    builder->appendFormat(" >> %d)", bitsToRead);
                }

                if (remainingBits == 0) {
                    remainingBits = 16;
                    builder->endOfStatement(true);

                    // update checksum
                    builder->target->emitTraceMessage(builder, "InternetChecksum: word=0x%llx", 1,
                                                      tmpVar.c_str());
                    builder->emitIndent();
                    if (addData) {
                        builder->appendFormat(
                            "bpf_p4tc_ext_csum_16bit_complement_add(&%s, "
                            "sizeof(%s), &%s, sizeof(%s))",
                            csumVar.c_str(), csumVar.c_str(), tmpVar.c_str(), tmpVar.c_str());
                    } else {
                        builder->appendFormat(
                            "bpf_p4tc_ext_csum_16bit_complement_sub(&%s, "
                            "sizeof(%s), &%s, sizeof(%s))",
                            csumVar.c_str(), csumVar.c_str(), tmpVar.c_str(), tmpVar.c_str());
                    }
                    builder->endOfStatement(true);
                }
            }
        }
    }

    builder->target->emitTraceMessage(builder, "InternetChecksum: new state=0x%llx", 1,
                                      stateVar.c_str());
    builder->blockEnd(true);
}

cstring InternetChecksumAlgorithmPNA::getConvertByteOrderFunction(unsigned widthToEmit,
                                                                  cstring byte_order) {
    cstring emit;
    if (widthToEmit <= 16) {
        emit = byte_order == "HOST" ? "bpf_ntohs"_cs : "bpf_htons"_cs;
    } else if (widthToEmit <= 32) {
        emit = byte_order == "HOST" ? "bpf_ntohl"_cs : "bpf_htonl"_cs;
    } else if (widthToEmit <= 64) {
        emit = byte_order == "HOST" ? "ntohll"_cs : "bpf_cpu_to_be64"_cs;
    }
    return emit;
}

void EBPFDigestPNA::emitInitializer(EBPF::CodeBuilder *builder) const {
    builder->newline();
    builder->emitIndent();
    builder->appendLine("__builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));");
    builder->emitIndent();
    builder->appendLine("ext_params.pipe_id = p4tc_filter_fields.pipeid;");
    builder->emitIndent();
    auto extId = tcIR->getExternId(externName);
    BUG_CHECK(!extId.isNullOrEmpty(), "Extern ID not found");
    builder->appendFormat("ext_params.ext_id = %s;", extId);
    builder->newline();
    builder->emitIndent();
    auto instId = tcIR->getExternInstanceId(externName, instanceName);
    BUG_CHECK(instId != 0, "Extern instance ID not found");
    builder->appendFormat("ext_params.inst_id = %d;", instId);
    builder->newline();
}

void EBPFDigestPNA::emitPushElement(EBPF::CodeBuilder *builder, const IR::Expression *elem,
                                    Inspector *codegen) const {
    emitInitializer(builder);
    builder->newline();
    builder->emitIndent();
    builder->append("__builtin_memcpy(ext_params.in_params, &");
    codegen->visit(elem);
    builder->append(", sizeof(");
    this->valueType->declare(builder, cstring::empty, false);
    builder->append("));");
    builder->newline();
    builder->emitIndent();
    builder->append("bpf_p4tc_extern_digest_pack(skb, &ext_params, sizeof(ext_params))");
}

void EBPFDigestPNA::emitPushElement(EBPF::CodeBuilder *builder, cstring elem) const {
    emitInitializer(builder);
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("__builtin_memcpy(ext_params.in_params, &%s, sizeof(", elem);
    this->valueType->declare(builder, cstring::empty, false);
    builder->append("));");
    builder->newline();
    builder->emitIndent();
    builder->append("bpf_p4tc_extern_digest_pack(skb, &ext_params, sizeof(ext_params));");
}

void EBPFMeterPNA::emitInitializer(EBPF::CodeBuilder *builder, const ConvertToBackendIR *tcIR,
                                   cstring externName) const {
    builder->emitIndent();
    builder->appendLine("__builtin_memset(&ext_params, 0, sizeof(struct p4tc_ext_bpf_params));");
    builder->emitIndent();
    builder->appendLine("ext_params.pipe_id = p4tc_filter_fields.pipeid;");
    builder->emitIndent();
    auto extId = tcIR->getExternId(externName);
    BUG_CHECK(!extId.isNullOrEmpty(), "Extern ID not found");
    builder->appendFormat("ext_params.ext_id = %s;", extId);
    builder->newline();
    cstring ext_flags = isDirect ? "P4TC_EXT_METER_DIRECT"_cs : "P4TC_EXT_METER_INDIRECT"_cs;
    builder->emitIndent();
    builder->appendFormat("ext_params.flags = %s;", ext_flags);
}

void EBPFMeterPNA::emitExecute(EBPF::CodeBuilder *builder, const P4::ExternMethod *method,
                               ControlBodyTranslatorPNA *translator,
                               const IR::Expression *leftExpression) const {
    auto externName = method->originalExternType->name.name;
    auto index = method->expr->arguments->at(0)->expression;
    auto instId = translator->tcIR->getExternInstanceId(externName, this->instanceName);
    bool isColorAware = false;

    emitInitializer(builder, translator->tcIR, externName);

    builder->newline();
    builder->emitIndent();
    BUG_CHECK(instId != 0, "Extern instance ID not found");
    builder->appendFormat("ext_params.inst_id = %d;", instId);
    builder->newline();
    builder->emitIndent();
    builder->append("ext_params.index = ");
    translator->visit(index);
    builder->endOfStatement(true);
    builder->emitIndent();

    if (method->expr->arguments->size() == 2) {
        isColorAware = true;
        auto value = method->expr->arguments->at(1)->expression;
        cstring color_aware = "color_meter"_cs;
        auto etype = EBPF::EBPFTypeFactory::instance->create(IR::Type_Bits::get(32));
        builder->newline();
        builder->emitIndent();
        etype->declare(builder, color_aware, true);
        builder->appendLine(" = (u32 *)ext_params.in_params;");
        builder->emitIndent();
        builder->appendFormat("*%s = ", color_aware);
        translator->visit(value);
        builder->endOfStatement();
    }

    if (leftExpression != nullptr) {
        builder->newline();
        builder->emitIndent();
        translator->visit(leftExpression);
        builder->append(" = ");
    }

    if (type == BYTES) {
        if (isColorAware) {
            builder->appendLine(
                "bpf_p4tc_extern_meter_bytes_color(skb, &ext_params, sizeof(ext_params), NULL, "
                "0);");
        } else {
            builder->appendLine(
                "bpf_p4tc_extern_meter_bytes(skb, &ext_params, sizeof(ext_params), NULL, 0);");
        }
    } else if (type == PACKETS) {
        if (isColorAware) {
            builder->appendLine(
                "bpf_p4tc_extern_meter_pkts_color(skb, &ext_params, sizeof(ext_params), NULL, 0);");
        } else {
            builder->appendLine(
                "bpf_p4tc_extern_meter_pkts(skb, &ext_params, sizeof(ext_params), NULL, 0);");
        }
    }
}

void EBPFMeterPNA::emitDirectMeterExecute(EBPF::CodeBuilder *builder,
                                          const P4::ExternMethod *method,
                                          ControlBodyTranslatorPNA *translator,
                                          const IR::Expression *leftExpression) const {
    auto externName = method->originalExternType->name.name;
    bool isColorAware = false;

    emitInitializer(builder, translator->tcIR, externName);

    builder->newline();
    auto tblId = translator->tcIR->getTableId(tblname);
    BUG_CHECK(tblId != 0, "Table ID not found");
    builder->emitIndent();
    builder->appendFormat("ext_params.tbl_id = %d;", tblId);
    builder->newline();

    if (method->expr->arguments->size() == 1) {
        isColorAware = true;
        auto value = method->expr->arguments->at(0)->expression;
        cstring color_aware = "color_meter"_cs;
        auto etype = EBPF::EBPFTypeFactory::instance->create(IR::Type_Bits::get(32));
        builder->newline();
        builder->emitIndent();
        etype->declare(builder, color_aware, true);
        builder->appendLine(" = (u32 *)ext_params.in_params;");
        builder->emitIndent();
        builder->appendFormat("*%s = ", color_aware);
        translator->visit(value);
        builder->endOfStatement();
    }

    builder->newline();
    builder->emitIndent();
    if (leftExpression != nullptr) {
        translator->visit(leftExpression);
        builder->append(" = ");
    }

    if (type == BYTES) {
        if (isColorAware) {
            builder->appendLine(
                "bpf_p4tc_extern_meter_bytes_color(skb, &ext_params, sizeof(ext_params), &key, "
                "sizeof(key));");
        } else {
            builder->appendLine(
                "bpf_p4tc_extern_meter_bytes(skb, &ext_params, sizeof(ext_params), &key, "
                "sizeof(key));");
        }
    } else if (type == PACKETS) {
        if (isColorAware) {
            builder->appendLine(
                "bpf_p4tc_extern_meter_pkts_color(skb, &ext_params, sizeof(ext_params), &key, "
                "sizeof(key));");
        } else {
            builder->appendLine(
                "bpf_p4tc_extern_meter_pkts(skb, &ext_params, sizeof(ext_params), &key, "
                "sizeof(key));");
        }
    }
}

}  // namespace P4::TC
