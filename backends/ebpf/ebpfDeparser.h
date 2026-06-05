/*
 * SPDX-FileCopyrightText: 2022 Open Networking Foundation
 * SPDX-FileCopyrightText: 2022 Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BACKENDS_EBPF_EBPFDEPARSER_H_
#define BACKENDS_EBPF_EBPFDEPARSER_H_

#include "ebpfControl.h"

namespace P4::EBPF {

class EBPFDeparser;

/// This translator emits deparser externs.
class DeparserBodyTranslator : public ControlBodyTranslator {
 protected:
    const EBPFDeparser *deparser;

 public:
    explicit DeparserBodyTranslator(const EBPFDeparser *deparser);

    bool preorder(const IR::MethodCallExpression *expression) override;
    bool preorder(const IR::IfStatement *ifstmt) override;
};

/// This translator emits buffer preparation (eg. which headers will be emitted)
class DeparserPrepareBufferTranslator : public ControlBodyTranslator {
 protected:
    const EBPFDeparser *deparser;

 public:
    explicit DeparserPrepareBufferTranslator(const EBPFDeparser *deparser);

    void processMethod(const P4::ExternMethod *method) override;
    bool preorder(const IR::BlockStatement *s) override;
    bool preorder(const IR::IfStatement *ifstmt) override;
    bool preorder(const IR::AssignmentStatement *) override { return false; }
    bool preorder(const IR::MethodCallStatement *s) override;
    bool preorder(const IR::MethodCallExpression *expression) override;
};

/// This translator emits headers
class DeparserHdrEmitTranslator : public DeparserPrepareBufferTranslator {
 protected:
    const EBPFDeparser *deparser;

 public:
    explicit DeparserHdrEmitTranslator(const EBPFDeparser *deparser);

    bool preorder(const IR::IfStatement *ifstmt) override;
    bool preorder(const IR::MethodCallExpression *expression) override;
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
    /// A "PreDeparser" is emitted just before a sequence of hdr.emit() functions.
    /// It is useful in the case of resubmit or clone operation, as these operations
    /// require to have an original packet.
    virtual void emitPreDeparser(CodeBuilder *builder) { (void)builder; }

    virtual void emitDeparserExternCalls(CodeBuilder *builder) { (void)builder; }

    void emitBufferAdjusts(CodeBuilder *builder) const;

    DECLARE_TYPEINFO(EBPFDeparser, EBPFControl);
};

}  // namespace P4::EBPF

#endif /* BACKENDS_EBPF_EBPFDEPARSER_H_ */
