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

#ifndef BACKENDS_UBPF_UBPFPROGRAM_H_
#define BACKENDS_UBPF_UBPFPROGRAM_H_

#include "backends/ebpf/ebpfOptions.h"
#include "backends/ebpf/ebpfProgram.h"
#include "codeGen.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "target.h"
#include "ubpfModel.h"

namespace p4c::UBPF {

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
    cstring packetTruncatedSizeVar;
    cstring arrayIndexType = "uint32_t"_cs;

    UBPFProgram(const EbpfOptions &options, const IR::P4Program *program, P4::ReferenceMap *refMap,
                P4::TypeMap *typeMap, const IR::ToplevelBlock *toplevel)
        : EBPF::EBPFProgram(options, program, refMap, typeMap, toplevel),
          model(UBPFModel::instance) {
        packetStartVar = cstring("pkt");
        offsetVar = cstring("packetOffsetInBits");
        outerHdrOffsetVar = cstring("outHeaderOffset");
        outerHdrLengthVar = cstring("outHeaderLength");
        contextVar = cstring("ctx");
        lengthVar = cstring("pkt_len");
        endLabel = cstring("deparser");
        stdMetadataVar = cstring("std_meta");
        packetTruncatedSizeVar = cstring("packetTruncatedSize");
    }

    bool build() override;
    void emitC(UbpfCodeBuilder *builder, const std::filesystem::path &headerFile);
    void emitH(EBPF::CodeBuilder *builder, const std::filesystem::path &headerFile) override;
    void emitPreamble(EBPF::CodeBuilder *builder) override;
    void emitTypes(EBPF::CodeBuilder *builder) override;
    void emitTableDefinition(EBPF::CodeBuilder *builder) const;
    void emitPktVariable(UbpfCodeBuilder *builder) const;
    void emitPacketLengthVariable(UbpfCodeBuilder *builder) const;
    void emitHeaderInstances(EBPF::CodeBuilder *builder) override;
    void emitMetadataInstance(EBPF::CodeBuilder *builder) const;
    void emitLocalVariables(EBPF::CodeBuilder *builder) override;
    void emitPipeline(EBPF::CodeBuilder *builder) override;

    bool isLibraryMethod(cstring methodName) override {
        static std::set<cstring> DEFAULT_METHODS = {
            "mark_to_drop"_cs, "mark_to_pass"_cs,  "ubpf_time_get_ns"_cs, "truncate"_cs,
            "hash"_cs,         "csum_replace2"_cs, "csum_replace4"_cs,
        };
        return DEFAULT_METHODS.find(methodName) != DEFAULT_METHODS.end() ||
               EBPFProgram::isLibraryMethod(methodName);
    }
};

}  // namespace p4c::UBPF

#endif /* BACKENDS_UBPF_UBPFPROGRAM_H_ */
