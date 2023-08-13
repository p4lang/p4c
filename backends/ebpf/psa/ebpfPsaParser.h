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
#ifndef BACKENDS_EBPF_PSA_EBPFPSAPARSER_H_
#define BACKENDS_EBPF_PSA_EBPFPSAPARSER_H_

#include <map>

#include "backends/ebpf/codeGen.h"
#include "backends/ebpf/ebpfParser.h"
#include "backends/ebpf/ebpfProgram.h"
#include "backends/ebpf/psa/externs/ebpfPsaChecksum.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/map.h"

namespace EBPF {

class EBPFPsaParser;

class PsaStateTranslationVisitor : public StateTranslationVisitor {
 public:
    EBPFPsaParser *parser;

    explicit PsaStateTranslationVisitor(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                                        EBPFPsaParser *prsr)
        : StateTranslationVisitor(refMap, typeMap), parser(prsr) {}

    void processMethod(const P4::ExternMethod *ext) override;
};

class EBPFPsaParser : public EBPFParser {
 public:
    std::map<cstring, EBPFChecksumPSA *> checksums;
    const IR::Parameter *inputMetadata;

    EBPFPsaParser(const EBPFProgram *program, const IR::ParserBlock *block,
                  const P4::TypeMap *typeMap);

    void emit(CodeBuilder *builder) override;
    void emitParserInputMetadata(CodeBuilder *builder);
    void emitDeclaration(CodeBuilder *builder, const IR::Declaration *decl) override;
    void emitRejectState(CodeBuilder *builder) override;

    EBPFChecksumPSA *getChecksum(cstring name) const {
        auto result = ::get(checksums, name);
        BUG_CHECK(result != nullptr, "No checksum named %1%", name);
        return result;
    }
};

}  // namespace EBPF

#endif /* BACKENDS_EBPF_PSA_EBPFPSAPARSER_H_ */
