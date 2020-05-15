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

#ifndef P4C_UBPFPROGRAM_H
#define P4C_UBPFPROGRAM_H

#include "target.h"
#include "ubpfModel.h"
#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "backends/ebpf/ebpfProgram.h"
#include "backends/ebpf/ebpfOptions.h"
#include "codeGen.h"


namespace UBPF {

    class UBPFControl;
    class UBPFParser;
    class UBPFDeparser;
    class UBPFTable;
    class UBPFType;

    class UBPFProgram : public EBPF::EBPFProgram {
    public:
        UBPFParser *parser{};
        UBPFControl *control{};
        UBPFDeparser *deparser{};
        UBPFModel &model;

        cstring contextVar, outerHdrOffsetVar, outerHdrLengthVar;
        cstring stdMetadataVar;

        UBPFProgram(const EbpfOptions &options, const IR::P4Program *program,
                    P4::ReferenceMap *refMap, P4::TypeMap *typeMap, const IR::ToplevelBlock *toplevel) :
                EBPF::EBPFProgram(options, program, refMap, typeMap, toplevel), model(UBPFModel::instance) {
            packetStartVar = cstring("pkt");
            offsetVar = cstring("packetOffsetInBits");
            outerHdrOffsetVar = cstring("outHeaderOffset");
            outerHdrLengthVar = cstring("outHeaderLength");
            contextVar = cstring("ctx");
            lengthVar = cstring("pkt_len");
            endLabel = cstring("deparser");
            stdMetadataVar = cstring("std_meta");
        }

        bool build() override;
        void emitC(UbpfCodeBuilder *builder, cstring headerFile);
        void emitH(EBPF::CodeBuilder *builder, cstring headerFile) override;
        void emitPreamble(EBPF::CodeBuilder *builder) override;
        void emitTypes(EBPF::CodeBuilder *builder) override;
        void emitTableDefinition(EBPF::CodeBuilder *builder) const;
        void emitPktVariable(UbpfCodeBuilder *builder) const;
        void emitPacketLengthVariable(UbpfCodeBuilder *builder) const;
        void emitHeaderInstances(EBPF::CodeBuilder *builder) override;
        void emitMetadataInstance(EBPF::CodeBuilder *builder) const;
        void emitLocalVariables(EBPF::CodeBuilder *builder) override;
        void emitPipeline(EBPF::CodeBuilder *builder) override;
    };

}

#endif
