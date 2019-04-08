#ifndef P4C_UBPFPARSER_H
#define P4C_UBPFPARSER_H

#include "ir/ir.h"
#include "backends/ebpf/ebpfParser.h"

namespace UBPF {

class UBPFParserState : public EBPF::EBPFParserState {
public:
    UBPFParserState(const IR::ParserState* state, EBPF::EBPFParser* parser) : EBPF::EBPFParserState(state, parser) {}
    void emit(EBPF::CodeBuilder* builder);
};

class UBPFParser : public EBPF::EBPFParser {
public:
    UBPFParser(const EBPF::EBPFProgram* program, const IR::ParserBlock* block,
               const P4::TypeMap* typeMap) : EBPF::EBPFParser(program, block, typeMap) {}
    bool build();
    void emit(EBPF::CodeBuilder* builder);
};

}

#endif //P4C_UBPFPARSER_H
