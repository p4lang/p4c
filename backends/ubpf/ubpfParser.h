/*
Copyright 2019 Orange

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

#ifndef P4C_UBPFPARSER_H
#define P4C_UBPFPARSER_H

#include "ir/ir.h"
#include "backends/ebpf/ebpfParser.h"
#include "ubpfType.h"

namespace UBPF {

    class UBPFParserState : public EBPF::EBPFParserState {
    public:
        UBPFParserState(const IR::ParserState *state, EBPF::EBPFParser *parser) : EBPF::EBPFParserState(state,
                                                                                                        parser) {}

        void emit(EBPF::CodeBuilder *builder);
    };

    class UBPFParser : public EBPF::EBPFParser {
    public:
        std::vector<UBPFParserState *> states;
        const IR::Parameter *metadata;
        EBPF::EBPFType *metadataType;

        UBPFParser(const EBPF::EBPFProgram *program, const IR::ParserBlock *block,
                   const P4::TypeMap *typeMap) : EBPF::EBPFParser(program, block, typeMap) {}

        void emit(EBPF::CodeBuilder *builder);
        bool build();
    };

}

#endif
