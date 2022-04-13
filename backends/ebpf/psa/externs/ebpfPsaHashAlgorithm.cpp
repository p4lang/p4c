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
#include "ebpfPsaHashAlgorithm.h"
#include "backends/ebpf/ebpfProgram.h"
#include "backends/ebpf/ebpfType.h"

namespace EBPF {

EBPFHashAlgorithmPSA::ArgumentsList EBPFHashAlgorithmPSA::unpackArguments(
        const IR::MethodCallExpression * expr, int dataPos) {
    BUG_CHECK(expr->arguments->size() > ((size_t) dataPos),
              "Data position %1% is outside of the arguments: %2%", dataPos, expr);

    std::vector<const IR::Expression *> arguments;

    if (auto argList = expr->arguments->at(dataPos)->expression->to<IR::StructExpression>()) {
        for (auto field : argList->components)
            arguments.push_back(field->expression);
    } else {
        arguments.push_back(expr->arguments->at(dataPos)->expression);
    }

    return arguments;
}

void EBPFHashAlgorithmPSA::emitSubtractData(CodeBuilder *builder, int dataPos,
                                            const IR::MethodCallExpression *expr) {
    emitSubtractData(builder, unpackArguments(expr, dataPos));
}

void EBPFHashAlgorithmPSA::emitAddData(CodeBuilder *builder, int dataPos,
                                       const IR::MethodCallExpression *expr) {
    emitAddData(builder, unpackArguments(expr, dataPos));
}

// ===========================CRCChecksumAlgorithm===========================

void CRCChecksumAlgorithm::emitUpdateMethod(CodeBuilder* builder, int crcWidth) {
    // Note that this update method is optimized for our CRC16 and CRC32, custom
    // version may require other method of update. To deal with byte order, data
    // is read from the end of buffer.
    cstring code = "static __always_inline\n"
                   "void crc%w%_update(u%w% * reg, const u8 * data, "
                   "u16 data_size, const u%w% poly) {\n"
                   "    data += data_size - 1;\n"
                   "    #pragma clang loop unroll(full)\n"
                   "    for (u16 i = 0; i < data_size; i++) {\n"
                   "        bpf_trace_message(\"CRC%w%: data byte: %x\\n\", *data);\n"
                   "        *reg ^= *data;\n"
                   "        for (u8 bit = 0; bit < 8; bit++) {\n"
                   "            *reg = (*reg) & 1 ? ((*reg) >> 1) ^ poly : (*reg) >> 1;\n"
                   "        }\n"
                   "        data--;\n"
                   "    }\n"
                   "}";
    code = code.replace("%w%", Util::printf_format("%d", crcWidth));
    builder->appendLine(code);
}

void CRCChecksumAlgorithm::emitVariables(CodeBuilder* builder,
                                         const IR::Declaration_Instance* decl) {
    registerVar = program->refMap->newName(baseName + "_reg");

    builder->emitIndent();

    if (decl != nullptr) {
        BUG_CHECK(decl->type->is<IR::Type_Specialized>(), "Must be a specialized type %1%", decl);
        auto ts = decl->type->to<IR::Type_Specialized>();
        BUG_CHECK(ts->arguments->size() == 1, "Expected 1 specialized type %1%", decl);

        auto otype = ts->arguments->at(0);
        if (!otype->is<IR::Type_Bits>()) {
            ::error(ErrorType::ERR_UNSUPPORTED, "Must be bit or int type: %1%", ts);
            return;
        }
        if (otype->width_bits() != crcWidth) {
            ::error(ErrorType::ERR_TYPE_ERROR, "Must be %1%-bits width: %2%", crcWidth, ts);
            return;
        }

        auto registerType = EBPFTypeFactory::instance->create(otype);
        registerType->emit(builder);
    } else {
        if (crcWidth == 16)
            builder->append("u16");
        else if (crcWidth == 32)
            builder->append("u32");
        else
            BUG("Unsupported CRC width %1%", crcWidth);
    }

    builder->appendFormat(" %s = %s", registerVar.c_str(), initialValue.c_str());
    builder->endOfStatement(true);
}

void CRCChecksumAlgorithm::emitClear(CodeBuilder* builder) {
    builder->emitIndent();
    builder->appendFormat("%s = %s", registerVar.c_str(), initialValue.c_str());
    builder->endOfStatement(true);
}

/*
 * This method generates a C code that is responsible for updating
 * a CRC check value from a given data.
 *
 * From following P4 code:
 * Checksum<bit<32>>(PSA_HashAlgorithm_t.CRC32) checksum;
 * checksum.update(parsed_hdr.crc.f1);
 * There will be generated a C code:
 * crc32_update(&c_0_reg, (u8 *) &(parsed_hdr->crc.f1), 5, 0xEDB88320);
 * Where:
 * c_0_reg - a checksum internal state (CRC register)
 * parsed_hdr->field1 - a data on which CRC is calculated
 * 5 - a field size in bytes
 * 0xEDB88320 - a polynomial in a reflected bit order.
 */
void CRCChecksumAlgorithm::emitAddData(CodeBuilder* builder,
                                       const ArgumentsList & arguments) {
    cstring tmpVar = program->refMap->newName(baseName + "_tmp");

    builder->emitIndent();
    builder->blockStart();

    builder->emitIndent();
    builder->appendFormat("u8 %s = 0", tmpVar.c_str());
    builder->endOfStatement(true);

    bool concatenateBits = false;
    int remainingBits = 8;
    for (auto field : arguments) {
        auto fieldType = field->type->to<IR::Type_Bits>();
        if (fieldType == nullptr) {
            ::error(ErrorType::ERR_UNSUPPORTED, "Only bits types are supported %1%", field);
            return;
        }
        const int width = fieldType->width_bits();

        // We concatenate less than 8-bit fields into one byte
        if (width < 8 || concatenateBits) {
            concatenateBits = true;
            if (width > remainingBits) {
                ::error(ErrorType::ERR_UNSUPPORTED,
                        "Unable to concatenate fields into one byte. "
                        "Last field(%1%) overflows one byte. "
                        "There are %2% remaining bits but field (%1%) has %3% bits width.",
                        field, remainingBits, width);
                return;
            }
            if (remainingBits == 8) {
                // start processing sub-byte fields
                builder->emitIndent();
                builder->appendFormat("%s = ", tmpVar.c_str());
            } else {
                builder->append(" | ");
            }

            remainingBits -= width;
            builder->append("(");
            visitor->visit(field);
            builder->appendFormat(" << %d)", remainingBits);

            if (remainingBits == 0) {
                // last bit, update the crc
                concatenateBits = false;
                builder->endOfStatement(true);
                builder->emitIndent();
                builder->appendFormat("%s(&%s, &%s, 1, %s)", updateMethod.c_str(),
                                      registerVar.c_str(), tmpVar.c_str(), polynomial.c_str());
                builder->endOfStatement(true);
            }
        } else {
            // fields larger than 8 bits
            if (width % 8 != 0) {
                ::error(ErrorType::ERR_UNSUPPORTED,
                        "Fields larger than 8 bits have to be aligned to bytes %1%", field);
                return;
            }
            builder->emitIndent();
            builder->appendFormat("%s(&%s, (u8 *) &(", updateMethod.c_str(), registerVar.c_str());
            visitor->visit(field);
            builder->appendFormat("), %d, %s)", width / 8, polynomial.c_str());
            builder->endOfStatement(true);
        }
    }

    cstring varStr = Util::printf_format("(u64) %s", registerVar.c_str());
    builder->target->emitTraceMessage(builder, "CRC: checksum state: %llx", 1, varStr.c_str());

    cstring final_crc = Util::printf_format("(u64) %s(%s)", finalizeMethod.c_str(),
                                            registerVar.c_str());
    builder->target->emitTraceMessage(builder, "CRC: final checksum: %llx", 1, final_crc.c_str());

    builder->blockEnd(true);
}

void CRCChecksumAlgorithm::emitGet(CodeBuilder* builder) {
    builder->appendFormat("%s(%s)", finalizeMethod.c_str(),
                          registerVar.c_str());
}

void CRCChecksumAlgorithm::emitSubtractData(CodeBuilder* builder,
                                            const ArgumentsList & arguments) {
    (void) builder; (void) arguments;
    BUG("Not implementable");
}

void CRCChecksumAlgorithm::emitGetInternalState(CodeBuilder* builder) {
    (void) builder;
    BUG("Not implemented");
}

void CRCChecksumAlgorithm::emitSetInternalState(CodeBuilder* builder,
                                                const IR::MethodCallExpression * expr) {
    (void) builder; (void) expr;
    BUG("Not implemented");
}

// ===========================CRC16ChecksumAlgorithm===========================

void CRC16ChecksumAlgorithm::emitGlobals(CodeBuilder* builder) {
    CRCChecksumAlgorithm::emitUpdateMethod(builder, 16);

    cstring code ="static __always_inline "
                  "u16 crc16_finalize(u16 reg) {\n"
                  "    return reg;\n"
                  "}";
    builder->appendLine(code);
}

// ===========================CRC32ChecksumAlgorithm===========================

void CRC32ChecksumAlgorithm::emitGlobals(CodeBuilder* builder) {
    CRCChecksumAlgorithm::emitUpdateMethod(builder, 32);

    cstring code = "static __always_inline "
                   "u32 crc32_finalize(u32 reg) {\n"
                   "    return reg ^ 0xFFFFFFFF;\n"
                   "}";
    builder->appendLine(code);
}

// ===========================InternetChecksumAlgorithm===========================

void InternetChecksumAlgorithm::updateChecksum(CodeBuilder* builder,
                                               const ArgumentsList & arguments,
                                               bool addData) {
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
            ::error(ErrorType::ERR_UNSUPPORTED, "Unsupported field type: %1%", field->type);
            return;
        }
        const int width = fieldType->width_bits();
        bitsToRead = width;

        if (width > 64) {
            BUG("Fields wider than 64 bits are not supported yet", field);
        }

        while (bitsToRead > 0) {
            if (remainingBits == 16) {
                builder->emitIndent();
                builder->appendFormat("%s = ", tmpVar.c_str());
            } else {
                builder->append(" | ");
            }

            // TODO: add masks for fields, however they should not exceed declared width
            if (bitsToRead < remainingBits) {
                remainingBits -= bitsToRead;
                builder->append("(");
                visitor->visit(field);
                builder->appendFormat(" << %d)", remainingBits);
                bitsToRead = 0;
            } else if (bitsToRead == remainingBits) {
                remainingBits = 0;
                visitor->visit(field);
                bitsToRead = 0;
            } else if (bitsToRead > remainingBits) {
                bitsToRead -= remainingBits;
                remainingBits = 0;
                builder->append("(");
                visitor->visit(field);
                builder->appendFormat(" >> %d)", bitsToRead);
            }

            if (remainingBits == 0) {
                remainingBits = 16;
                builder->endOfStatement(true);

                // update checksum
                builder->target->emitTraceMessage(builder, "InternetChecksum: word=0x%llx",
                                                  1, tmpVar.c_str());
                builder->emitIndent();
                if (addData) {
                    builder->appendFormat("%s = csum16_add(%s, %s)", stateVar.c_str(),
                                          stateVar.c_str(), tmpVar.c_str());
                } else {
                    builder->appendFormat("%s = csum16_sub(%s, %s)", stateVar.c_str(),
                                          stateVar.c_str(), tmpVar.c_str());
                }
                builder->endOfStatement(true);
            }
        }
    }

    builder->target->emitTraceMessage(builder, "InternetChecksum: new state=0x%llx",
                                      1, stateVar.c_str());
    builder->blockEnd(true);
}

void InternetChecksumAlgorithm::emitGlobals(CodeBuilder* builder) {
    builder->appendLine("inline u16 csum16_add(u16 csum, u16 addend) {\n"
                        "    u16 res = csum;\n"
                        "    res += addend;\n"
                        "    return (res + (res < addend));\n"
                        "}\n"
                        "inline u16 csum16_sub(u16 csum, u16 addend) {\n"
                        "    return csum16_add(csum, ~addend);\n"
                        "}");
}

void InternetChecksumAlgorithm::emitVariables(CodeBuilder* builder,
                                              const IR::Declaration_Instance* decl) {
    (void) decl;
    stateVar = program->refMap->newName(baseName + "_state");
    builder->emitIndent();
    builder->appendFormat("u16 %s = 0", stateVar.c_str());
    builder->endOfStatement(true);
}

void InternetChecksumAlgorithm::emitClear(CodeBuilder* builder) {
    builder->emitIndent();
    builder->appendFormat("%s = 0", stateVar.c_str());
    builder->endOfStatement(true);
}

void InternetChecksumAlgorithm::emitAddData(CodeBuilder* builder,
                                            const ArgumentsList & arguments) {
    updateChecksum(builder, arguments, true);
}

void InternetChecksumAlgorithm::emitGet(CodeBuilder* builder) {
    builder->appendFormat("((u16) (~%s))", stateVar.c_str());
}

void InternetChecksumAlgorithm::emitSubtractData(CodeBuilder* builder,
                                                 const ArgumentsList & arguments) {
    updateChecksum(builder, arguments, false);
}

void InternetChecksumAlgorithm::emitGetInternalState(CodeBuilder* builder) {
    builder->append(stateVar);
}

void InternetChecksumAlgorithm::emitSetInternalState(CodeBuilder* builder,
                                                     const IR::MethodCallExpression * expr) {
    if (expr->arguments->size() != 1) {
        ::error(ErrorType::ERR_UNEXPECTED, "Expected exactly 1 argument %1%", expr);
        return;
    }
    builder->emitIndent();
    builder->appendFormat("%s = ", stateVar.c_str());
    visitor->visit(expr->arguments->at(0)->expression);
    builder->endOfStatement(true);
}

}  // namespace EBPF
