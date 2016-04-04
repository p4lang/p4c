#ifndef _BACKENDS_EBPF_EBPFPARSER_H_
#define _BACKENDS_EBPF_EBPFPARSER_H_

#include "ir/ir.h"
#include "ebpfObject.h"

namespace EBPF {

class EBPFParserState : public EBPFObject {
 public:
    const IR::ParserState* state;
    const EBPFParser* parser;

    EBPFParserState(const IR::ParserState* state, EBPFParser* parser) :
            state(state), parser(parser) {}
    void emit(CodeBuilder* builder) override;
};

class EBPFParser : public EBPFObject {
 public:
    const EBPFProgram*            program;
    const P4::TypeMap*            typeMap;
    const IR::ParserBlock*        parserBlock;
    std::vector<EBPFParserState*> states;
    const IR::Parameter*          packet;
    const IR::Parameter*          headers;
    EBPFType*                     headerType;

    explicit EBPFParser(const EBPFProgram* program, const IR::ParserBlock* block,
                        const P4::TypeMap* typeMap);
    void emit(CodeBuilder* builder) override;
    bool build();
};

}  // namespace EBPF

#endif /* _BACKENDS_EBPF_EBPFPARSER_H_ */
