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

#include "backends/ebpf/ebpfType.h"
#include "backends/ebpf/ebpfParser.h"
#include "backends/ebpf/psa/ebpfPsaTable.h"

namespace EBPF {

class EBPFPsaParser;

class PsaStateTranslationVisitor : public StateTranslationVisitor {
 public:
    EBPFPsaParser * parser;

    const IR::SelectExpression* currentSelectExpression = nullptr;

    explicit PsaStateTranslationVisitor(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
                                        EBPFPsaParser * prsr) :
        StateTranslationVisitor(refMap, typeMap), parser(prsr) {}

    bool preorder(const IR::Expression* expression) override;
    bool preorder(const IR::Mask* expression) override;

    void processFunction(const P4::ExternFunction* function) override;
    void processMethod(const P4::ExternMethod* ext) override;

    void compileVerify(const IR::MethodCallExpression * expression);
};

class EBPFPsaParser : public EBPFParser {
 public:
    EBPFPsaParser(const EBPFProgram* program, const IR::ParserBlock* block,
                  const P4::TypeMap* typeMap);

    void emitRejectState(CodeBuilder* builder) override;
};

}  // namespace EBPF

#endif  /* BACKENDS_EBPF_PSA_EBPFPSAPARSER_H_ */
