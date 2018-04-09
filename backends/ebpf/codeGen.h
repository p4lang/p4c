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

#ifndef _BACKENDS_EBPF_CODEGEN_H_
#define _BACKENDS_EBPF_CODEGEN_H_

#include "ir/ir.h"
#include "lib/sourceCodeBuilder.h"
#include "target.h"
#include "frontends/p4/typeMap.h"

namespace P4 {

class ReferenceMap;

}

namespace EBPF {

class CodeBuilder : public Util::SourceCodeBuilder {
 public:
    const Target* target;
    explicit CodeBuilder(const Target* target) : target(target) {}
};

// Visitor for generating C for EBPF
// This visitor is invoked on various subtrees
class CodeGenInspector : public Inspector {
 protected:
    CodeBuilder*       builder;
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
    std::map<const IR::Parameter*, const IR::Parameter*> substitution;

 public:
    CodeGenInspector(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) :
            builder(nullptr), refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        visitDagOnce = false;
    }

    void setBuilder(CodeBuilder* builder) {
        CHECK_NULL(builder);
        this->builder = builder;
    }

    void substitute(const IR::Parameter* p, const IR::Parameter* with);
    void copySubstitutions(CodeGenInspector* other) {
        for (auto s : other->substitution)
            substitute(s.first, s.second);
    }

    bool notSupported(const IR::Expression* expression)
    { ::error("%1%: not yet implemented", expression); return false; }

    bool preorder(const IR::Expression* expression) override
    { return notSupported(expression); }
    bool preorder(const IR::Range* expression) override
    { return notSupported(expression); }
    bool preorder(const IR::Mask* expression) override
    { return notSupported(expression); }
    bool preorder(const IR::Slice* expression) override  // should not happen
    { return notSupported(expression); }
    bool preorder(const IR::StringLiteral* expression) override;
    bool preorder(const IR::ListExpression* expression) override;
    bool preorder(const IR::PathExpression* expression) override;
    bool preorder(const IR::Constant* expression) override;
    bool preorder(const IR::Declaration_Variable* decl) override;
    bool preorder(const IR::BoolLiteral* b) override;
    bool preorder(const IR::Cast* c) override;
    bool preorder(const IR::Operation_Binary* b) override;
    bool preorder(const IR::Operation_Unary* u) override;
    bool preorder(const IR::ArrayIndex* a) override;
    bool preorder(const IR::Mux* a) override;
    bool preorder(const IR::Member* e) override;
    bool preorder(const IR::MethodCallExpression* expression) override;
    bool comparison(const IR::Operation_Relation* comp);
    bool preorder(const IR::Equ* e) override { return comparison(e); }
    bool preorder(const IR::Neq* e) override { return comparison(e); }
    bool preorder(const IR::Path* path) override;

    bool preorder(const IR::Type_Typedef* type) override;
    bool preorder(const IR::Type_Enum* type) override;
    bool preorder(const IR::AssignmentStatement* s) override;
    bool preorder(const IR::BlockStatement* s) override;
    bool preorder(const IR::MethodCallStatement* s) override;
    bool preorder(const IR::EmptyStatement* s) override;
    bool preorder(const IR::ReturnStatement* s) override;
    bool preorder(const IR::ExitStatement* s) override;
    bool preorder(const IR::IfStatement* s) override;

    void widthCheck(const IR::Node* node) const;
};

}  // namespace EBPF


#endif /* _BACKENDS_EBPF_CODEGEN_H_ */
