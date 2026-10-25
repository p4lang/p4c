/*
 * Copyright 2019 Orange
 * SPDX-FileCopyrightText: 2019 Orange
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_UBPF_UBPFDEPARSER_H_
#define BACKENDS_UBPF_UBPFDEPARSER_H_

#include "backends/ebpf/ebpfObject.h"
#include "ir/ir.h"
#include "ubpfHelpers.h"
#include "ubpfProgram.h"

namespace P4::UBPF {

class UBPFDeparser;

class UBPFDeparserTranslationVisitor : public EBPF::CodeGenInspector {
 public:
    const UBPFDeparser *deparser;
    P4::P4CoreLibrary &p4lib;

    explicit UBPFDeparserTranslationVisitor(const UBPFDeparser *deparser);

    virtual void compileEmitField(const IR::Expression *expr, cstring field, unsigned alignment,
                                  EBPF::EBPFType *type);
    virtual void compileEmit(const IR::Vector<IR::Argument> *args);

    bool notSupported(const IR::Expression *expression) {
        ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "%1%: not supported in Deparser",
                    expression);
        return false;
    }

    bool notSupported(const IR::StatOrDecl *statOrDecl) {
        ::P4::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "%1%: not supported in Deparser",
                    statOrDecl);
        return false;
    }

    bool preorder(const IR::MethodCallExpression *expression) override;
    bool preorder(const IR::BaseAssignmentStatement *a) override { return notSupported(a); };
    bool preorder(const IR::ExitStatement *s) override { return notSupported(s); };
    bool preorder(UNUSED const IR::BlockStatement *s) override { return true; };
    bool preorder(const IR::ReturnStatement *s) override { return notSupported(s); };
    bool preorder(const IR::IfStatement *statement) override { return notSupported(statement); };
    bool preorder(const IR::SwitchStatement *statement) override {
        return notSupported(statement);
    };
    bool preorder(const IR::Operation_Binary *b) override { return notSupported(b); };
};

class UBPFDeparser : public EBPF::EBPFObject {
 public:
    const UBPFProgram *program;
    const IR::ControlBlock *controlBlock;
    const IR::Parameter *packet_out;
    const IR::Parameter *headers;
    const IR::Parameter *parserHeaders;

    UBPFDeparserTranslationVisitor *codeGen;

    UBPFDeparser(const UBPFProgram *program, const IR::ControlBlock *block,
                 const IR::Parameter *parserHeaders)
        : program(program), controlBlock(block), headers(nullptr), parserHeaders(parserHeaders) {}

    bool build();
    void emit(EBPF::CodeBuilder *builder);
};
}  // namespace P4::UBPF

#endif /* BACKENDS_UBPF_UBPFDEPARSER_H_ */
