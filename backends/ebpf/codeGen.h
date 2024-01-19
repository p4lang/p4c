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

#ifndef BACKENDS_EBPF_CODEGEN_H_
#define BACKENDS_EBPF_CODEGEN_H_

#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/sourceCodeBuilder.h"
#include "target.h"

namespace P4 {

class ReferenceMap;

}

namespace EBPF {

class CodeBuilder : public Util::SourceCodeBuilder {
 public:
    const Target *target;
    explicit CodeBuilder(const Target *target) : target(target) {}
};

// Visitor for generating C for EBPF
// This visitor is invoked on various subtrees
class CodeGenInspector : public Inspector {
 protected:
    CodeBuilder *builder;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    std::map<const IR::Parameter *, const IR::Parameter *> substitution;
    // asPointerVariables stores the list of string expressions that
    // should be emitted as pointer variables.
    std::set<cstring> asPointerVariables;

    // Since CodeGenInspector also generates C comments,
    // this variable keeps track of the current comment depth.
    int commentDescriptionDepth = 0;

 public:
    int expressionPrecedence;  /// precedence of current IR::Operation
    CodeGenInspector(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : builder(nullptr),
          refMap(refMap),
          typeMap(typeMap),
          expressionPrecedence(DBPrint::Prec_Low) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        visitDagOnce = false;
    }

    void setBuilder(CodeBuilder *builder) {
        CHECK_NULL(builder);
        this->builder = builder;
    }

    void substitute(const IR::Parameter *p, const IR::Parameter *with);
    void copySubstitutions(CodeGenInspector *other) {
        for (auto s : other->substitution) substitute(s.first, s.second);
    }

    void useAsPointerVariable(cstring name) { this->asPointerVariables.insert(name); }
    void copyPointerVariables(CodeGenInspector *other) {
        for (auto s : other->asPointerVariables) {
            this->asPointerVariables.insert(s);
        }
    }
    bool isPointerVariable(cstring name) { return asPointerVariables.count(name) > 0; }

    bool notSupported(const IR::Expression *expression) {
        ::error(ErrorType::ERR_UNSUPPORTED, "%1%: not yet implemented", expression);
        return false;
    }

    bool preorder(const IR::Expression *expression) override { return notSupported(expression); }
    bool preorder(const IR::Range *expression) override { return notSupported(expression); }
    bool preorder(const IR::Mask *expression) override { return notSupported(expression); }
    bool preorder(const IR::Slice *expression) override  // should not happen
    {
        return notSupported(expression);
    }
    bool preorder(const IR::StringLiteral *expression) override;
    bool preorder(const IR::ListExpression *expression) override;
    bool preorder(const IR::PathExpression *expression) override;
    bool preorder(const IR::Constant *expression) override;
    bool preorder(const IR::Declaration_Variable *decl) override;
    bool preorder(const IR::BoolLiteral *b) override;
    bool preorder(const IR::Cast *c) override;
    bool preorder(const IR::Operation_Binary *b) override;
    bool preorder(const IR::Operation_Unary *u) override;
    bool preorder(const IR::ArrayIndex *a) override;
    bool preorder(const IR::Mux *a) override;
    bool preorder(const IR::Member *e) override;
    bool preorder(const IR::MethodCallExpression *expression) override;
    bool comparison(const IR::Operation_Relation *comp);
    bool preorder(const IR::Equ *e) override { return comparison(e); }
    bool preorder(const IR::Neq *e) override { return comparison(e); }
    bool preorder(const IR::Path *path) override;
    bool preorder(const IR::StructExpression *expr) override;

    bool preorder(const IR::Type_Typedef *type) override;
    bool preorder(const IR::Type_Enum *type) override;
    void emitAssignStatement(const IR::Type *ltype, const IR::Expression *lexpr, cstring lpath,
                             const IR::Expression *rexpr);
    bool preorder(const IR::AssignmentStatement *s) override;
    bool preorder(const IR::BlockStatement *s) override;
    bool preorder(const IR::MethodCallStatement *s) override;
    bool preorder(const IR::EmptyStatement *s) override;
    bool preorder(const IR::ReturnStatement *s) override;
    bool preorder(const IR::ExitStatement *s) override;
    bool preorder(const IR::IfStatement *s) override;

    void widthCheck(const IR::Node *node) const;
};

class EBPFInitializerUtils {
 public:
    // return *real* number of bits required by type
    static unsigned ebpfTypeWidth(P4::TypeMap *typeMap, const IR::Expression *expr);

    // Generate hex string and prepend it with zeroes when shorter than required width
    static cstring genHexStr(const big_int &value, unsigned width, const IR::Expression *expr);
};

}  // namespace EBPF

#endif /* BACKENDS_EBPF_CODEGEN_H_ */
