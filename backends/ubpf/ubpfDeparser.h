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

#ifndef P4C_UBPFDEPARSER_H
#define P4C_UBPFDEPARSER_H

#include "ir/ir.h"
#include "ubpfProgram.h"
#include "ebpf/ebpfObject.h"
#include "ubpfHelpers.h"

namespace UBPF {

    class UBPFDeparser;

    class UBPFDeparserTranslationVisitor : public EBPF::CodeGenInspector {
    public:
        const UBPFDeparser *deparser;
        P4::P4CoreLibrary &p4lib;

        explicit UBPFDeparserTranslationVisitor(const UBPFDeparser *deparser);

        virtual void compileEmitField(const IR::Expression *expr, cstring field,
                                      unsigned alignment, EBPF::EBPFType *type);
        virtual void compileEmit(const IR::Vector<IR::Argument> *args);

        bool notSupported(const IR::Expression* expression)
        { ::error("%1%: not supported in Deparser", expression); return false; }

        bool notSupported(const IR::StatOrDecl* statOrDecl)
        { ::error("%1%: not supported in Deparser", statOrDecl); return false; }

        bool preorder(const IR::MethodCallExpression *expression) override;
        bool preorder(const IR::AssignmentStatement *a) override { return notSupported(a); };
        bool preorder(const IR::ExitStatement *s) override { return notSupported(s); };
        bool preorder(UNUSED const IR::BlockStatement *s) override { return true; };
        bool preorder(const IR::ReturnStatement *s) override { return notSupported(s); };
        bool preorder(const IR::IfStatement *statement) override { return notSupported(statement); };
        bool preorder(const IR::SwitchStatement *statement) override { return notSupported(statement); };
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
                     const IR::Parameter *parserHeaders) :
                program(program), controlBlock(block), headers(nullptr),
                parserHeaders(parserHeaders) {}

        bool build();
        void emit(EBPF::CodeBuilder *builder);
    };

}

#endif //P4C_UBPFDEPARSER_H
