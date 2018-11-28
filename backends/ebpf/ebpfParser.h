/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef _BACKENDS_EBPF_EBPFPARSER_H_
#define _BACKENDS_EBPF_EBPFPARSER_H_

#include "ir/ir.h"
#include "ebpfObject.h"
#include "ebpfProgram.h"

namespace EBPF {

class EBPFParser;

class EBPFParserState : public EBPFObject {
 public:
    const IR::ParserState* state;
    const EBPFParser* parser;

    EBPFParserState(const IR::ParserState* state, EBPFParser* parser) :
            state(state), parser(parser) {}
    void emit(CodeBuilder* builder);
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
    void emitDeclaration(CodeBuilder* builder, const IR::Declaration* decl);
    void emit(CodeBuilder* builder);
    bool build();
};

}  // namespace EBPF

#endif /* _BACKENDS_EBPF_EBPFPARSER_H_ */
