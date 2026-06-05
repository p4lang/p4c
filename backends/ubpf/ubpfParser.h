/*
 * Copyright 2019 Orange
 * SPDX-FileCopyrightText: 2019 Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_UBPF_UBPFPARSER_H_
#define BACKENDS_UBPF_UBPFPARSER_H_

#include "backends/ebpf/ebpfParser.h"
#include "ir/ir.h"
#include "ubpfType.h"

namespace P4::UBPF {

class UBPFParserState : public EBPF::EBPFParserState {
 public:
    UBPFParserState(const IR::ParserState *state, EBPF::EBPFParser *parser)
        : EBPF::EBPFParserState(state, parser) {}
    void emit(EBPF::CodeBuilder *builder);
};

class UBPFParser : public EBPF::EBPFParser {
 public:
    std::vector<UBPFParserState *> states;
    const IR::Parameter *metadata;
    EBPF::EBPFType *metadataType;

    UBPFParser(const EBPF::EBPFProgram *program, const IR::ParserBlock *block,
               const P4::TypeMap *typeMap)
        : EBPF::EBPFParser(program, block, typeMap) {}

    void emit(EBPF::CodeBuilder *builder);
    bool build();
};

}  // namespace P4::UBPF

#endif /* BACKENDS_UBPF_UBPFPARSER_H_ */
