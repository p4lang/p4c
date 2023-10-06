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
#ifndef BACKENDS_EBPF_EBPFDEPARSER_H_
#define BACKENDS_EBPF_EBPFDEPARSER_H_

#include "ebpfControl.h"

namespace EBPF {

class EBPFDeparser;

// this translator emits deparser externs
class DeparserBodyTranslator : public ControlBodyTranslator {
 protected:
    const EBPFDeparser *deparser;

 public:
    explicit DeparserBodyTranslator(const EBPFDeparser *deparser);

    bool preorder(const IR::MethodCallExpression *expression) override;
};

// this translator emits buffer preparation (eg. which headers will be emitted)
class DeparserPrepareBufferTranslator : public ControlBodyTranslator {
 protected:
    const EBPFDeparser *deparser;

 public:
    explicit DeparserPrepareBufferTranslator(const EBPFDeparser *deparser);

    void processMethod(const P4::ExternMethod *method) override;
    bool preorder(const IR::BlockStatement *s) override;
    bool preorder(const IR::MethodCallExpression *expression) override;
};

// this translator emits headers
class DeparserHdrEmitTranslator : public DeparserPrepareBufferTranslator {
 protected:
    const EBPFDeparser *deparser;

 public:
    explicit DeparserHdrEmitTranslator(const EBPFDeparser *deparser);

    void processMethod(const P4::ExternMethod *method) override;
    void emitField(CodeBuilder *builder, cstring field, const IR::Expression *hdrExpr,
                   unsigned alignment, EBPF::EBPFType *type);
};

class EBPFDeparser : public EBPFControl {
 public:
    const IR::Parameter *packet_out;

    EBPFType *headerType;
    cstring outerHdrOffsetVar, outerHdrLengthVar;
    cstring returnCode;

    EBPFDeparser(const EBPFProgram *program, const IR::ControlBlock *control,
                 const IR::Parameter *parserHeaders)
        : EBPFControl(program, control, parserHeaders) {
        codeGen = new DeparserBodyTranslator(this);
        outerHdrOffsetVar = cstring("outHeaderOffset");
        outerHdrLengthVar = cstring("outHeaderLength");
        returnCode = cstring("returnCode");
    }

    bool build() override;
    void emit(CodeBuilder *builder) override;
    // A "PreDeparser" is emitted just before a sequence of hdr.emit() functions.
    // It is useful in the case of resubmit or clone operation, as these operations
    // require to have an original packet.
    virtual void emitPreDeparser(CodeBuilder *builder) { (void)builder; }

    virtual void emitDeparserExternCalls(CodeBuilder *builder) { (void)builder; }

    void emitBufferAdjusts(CodeBuilder *builder) const;
};

}  // namespace EBPF

#endif /* BACKENDS_EBPF_EBPFDEPARSER_H_ */
