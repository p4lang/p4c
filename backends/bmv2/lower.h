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

#ifndef _BACKENDS_BMV2_LOWER_H_
#define _BACKENDS_BMV2_LOWER_H_

#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace BMV2 {

// Convert expressions not supported on BMv2
class LowerExpressions : public Transform {
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
    // Cannot shift with a value larger than 8 bits
    const int maxShiftWidth = 8;

    const IR::Expression* shift(const IR::Operation_Binary* expression) const;
 public:
    explicit LowerExpressions(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("LowerExpressions"); }

    const IR::Node* postorder(IR::Expression* expression) override;
    const IR::Node* postorder(IR::Shl* expression) override
    { return shift(expression); }
    const IR::Node* postorder(IR::Shr* expression) override
    { return shift(expression); }
    const IR::Node* postorder(IR::Cast* expression) override;
    const IR::Node* postorder(IR::Neg* expression) override;
    const IR::Node* postorder(IR::Slice* expression) override;
    const IR::Node* postorder(IR::Concat* expression) override;
    const IR::Node* preorder(IR::P4Table* table) override
    { prune(); return table; }  // don't simplify expressions in table
};

// This pass is a hack to work around current BMv2 limitations:
// checksum computations must be expressed in a restricted way, since
// the JSON code generator uses simple pattern-matching.
class FixupChecksum : public Transform {
    const cstring* updateBlockName;
 public:
    explicit FixupChecksum(const cstring* updateBlockName) :
            updateBlockName(updateBlockName)
    { setName("FixupChecksum"); }
    const IR::Node* preorder(IR::P4Control* control) override;
};


// If a select contains an expression which
// looks too complicated, it is lifted into a temporary.
class RemoveExpressionsFromSelects : public Transform {
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
    IR::IndexedVector<IR::Declaration> newDecls;
    IR::IndexedVector<IR::StatOrDecl>  assignments;

 public:
    RemoveExpressionsFromSelects(P4::ReferenceMap* refMap, P4::TypeMap* typeMap):
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("RemoveExpressionsFromSelects"); }
    const IR::Node* postorder(IR::SelectExpression* expression) override;
    const IR::Node* preorder(IR::ParserState* state) override
    { assignments.clear(); return state; }
    const IR::Node* postorder(IR::ParserState* state) override {
        if (!assignments.empty()) {
            auto comp = new IR::IndexedVector<IR::StatOrDecl>(*state->components);
            comp->append(assignments);
            state->components = comp;
        }
        return state;
    }
    const IR::Node* preorder(IR::P4Parser* parser) override
    { newDecls.clear(); return parser; }
    const IR::Node* postorder(IR::P4Parser* parser) override {
        if (!newDecls.empty()) {
            auto locals = new IR::IndexedVector<IR::Declaration>(*parser->parserLocals);
            locals->append(newDecls);
            parser->parserLocals = locals;
        }
        return parser;
    }
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_LOWER_H_ */
