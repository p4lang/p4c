/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef _BACKENDS_EBPF_EBPFPARSER_H_
#define _BACKENDS_EBPF_EBPFPARSER_H_

#include "ebpfObject.h"
#include "ebpfProgram.h"
#include "ebpfTable.h"
#include "frontends/p4/methodInstance.h"
#include "ir/ir.h"

namespace EBPF {

class EBPFParser;
class EBPFParserState;

class StateTranslationVisitor : public CodeGenInspector {
 protected:
    // stores the result of evaluating the select argument
    cstring selectValue;

    P4::P4CoreLibrary &p4lib;
    const EBPFParserState *state;

    void compileExtractField(const IR::Expression *expr, const IR::StructField *field,
                             unsigned alignment, EBPFType *type);
    virtual void compileExtract(const IR::Expression *destination);
    void compileLookahead(const IR::Expression *destination);
    void compileAdvance(const P4::ExternMethod *ext);
    void compileVerify(const IR::MethodCallExpression *expression);

    virtual void processFunction(const P4::ExternFunction *function);
    virtual void processMethod(const P4::ExternMethod *method);

 public:
    explicit StateTranslationVisitor(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : CodeGenInspector(refMap, typeMap), p4lib(P4::P4CoreLibrary::instance()), state(nullptr) {}

    void setState(const EBPFParserState *state) { this->state = state; }
    bool preorder(const IR::ParserState *state) override;
    bool preorder(const IR::SelectCase *selectCase) override;
    bool preorder(const IR::SelectExpression *expression) override;
    bool preorder(const IR::Member *expression) override;
    bool preorder(const IR::MethodCallExpression *expression) override;
    bool preorder(const IR::MethodCallStatement *stat) override {
        visit(stat->methodCall);
        builder->endOfStatement(true);
        return false;
    }
    bool preorder(const IR::AssignmentStatement *stat) override;
};

class EBPFParserState : public EBPFObject {
 public:
    const IR::ParserState *state;
    const EBPFParser *parser;

    EBPFParserState(const IR::ParserState *state, EBPFParser *parser)
        : state(state), parser(parser) {}
    void emit(CodeBuilder *builder);
};

class EBPFParser : public EBPFObject {
 public:
    const EBPFProgram *program;
    const P4::TypeMap *typeMap;
    const IR::ParserBlock *parserBlock;
    std::vector<EBPFParserState *> states;
    const IR::Parameter *packet;
    const IR::Parameter *headers;
    const IR::Parameter *user_metadata;
    EBPFType *headerType;

    StateTranslationVisitor *visitor;

    std::map<cstring, EBPFValueSet *> valueSets;

    explicit EBPFParser(const EBPFProgram *program, const IR::ParserBlock *block,
                        const P4::TypeMap *typeMap);
    virtual void emitDeclaration(CodeBuilder *builder, const IR::Declaration *decl);
    virtual void emit(CodeBuilder *builder);
    virtual bool build();

    virtual void emitTypes(CodeBuilder *builder);
    virtual void emitValueSetInstances(CodeBuilder *builder);
    virtual void emitRejectState(CodeBuilder *builder);

    EBPFValueSet *getValueSet(cstring name) const { return ::get(valueSets, name); }
};

}  // namespace EBPF

#endif /* _BACKENDS_EBPF_EBPFPARSER_H_ */
