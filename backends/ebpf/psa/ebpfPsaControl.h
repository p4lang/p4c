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
#ifndef BACKENDS_EBPF_PSA_EBPFPSACONTROL_H_
#define BACKENDS_EBPF_PSA_EBPFPSACONTROL_H_

#include "ebpfPsaTable.h"
#include "backends/ebpf/ebpfControl.h"

namespace EBPF {

class EBPFControlPSA;

class ControlBodyTranslatorPSA : public ControlBodyTranslator {
 public:
    explicit ControlBodyTranslatorPSA(const EBPFControlPSA* control);

    void processMethod(const P4::ExternMethod* method) override;
};

class ActionTranslationVisitorPSA : public ActionTranslationVisitor,
                                    public ControlBodyTranslatorPSA {
 public:
    ActionTranslationVisitorPSA(const EBPFProgram* program, cstring valueName);

    bool preorder(const IR::PathExpression* pe) override;
    bool isActionParameter(const IR::Expression *expression) const;

    void processMethod(const P4::ExternMethod* method) override;
};

class EBPFControlPSA : public EBPFControl {
 public:
    // Keeps track if ingress_timestamp or egress_timestamp is used within a control block.
    bool timestampIsUsed = false;

    const IR::Parameter* user_metadata;
    const IR::Parameter* inputStandardMetadata;
    const IR::Parameter* outputStandardMetadata;

    EBPFControlPSA(const EBPFProgram* program, const IR::ControlBlock* control,
                   const IR::Parameter* parserHeaders) :
        EBPFControl(program, control, parserHeaders) {}
};

}  // namespace EBPF

#endif  /* BACKENDS_EBPF_PSA_EBPFPSACONTROL_H_ */
