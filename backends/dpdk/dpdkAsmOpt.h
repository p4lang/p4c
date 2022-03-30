/*
Copyright 2020 Intel Corp.

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

#ifndef BACKENDS_DPDK_DPDKASMOPT_H_
#define BACKENDS_DPDK_DPDKASMOPT_H_

#include "frontends/common/constantFolding.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/enumInstance.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/unusedDeclarations.h"
#include "ir/ir.h"
#include "lib/gmputil.h"
#include "lib/json.h"

#define DPDK_TABLE_MAX_KEY_SIZE 64*8

namespace DPDK {
// This pass removes label that no jmps jump to
class RemoveRedundantLabel : public Transform {
 public:
    const IR::IndexedVector<IR::DpdkAsmStatement> *removeRedundantLabel(
                      const IR::IndexedVector<IR::DpdkAsmStatement> &s);

    const IR::Node *postorder(IR::DpdkListStatement *l) override {
        const IR::IndexedVector<IR::DpdkAsmStatement> *newStmts;
        newStmts = removeRedundantLabel(l->statements);
        l->statements = *newStmts;
        return l;
    }

    const IR::Node *postorder(IR::DpdkAction *l) override {
        const IR::IndexedVector<IR::DpdkAsmStatement> *newStmts;
        newStmts = removeRedundantLabel(l->statements);
        l->statements = *newStmts;
        return l;
    }
};

// This pass removes jmps that jump to a label that is immediately after it.
// For example,
// (jmp label1)
// (label1)
//
// jmp label1 will be removed.

class RemoveConsecutiveJmpAndLabel : public Transform {
 public:
    const IR::IndexedVector<IR::DpdkAsmStatement> *removeJmpAndLabel(
                   const IR::IndexedVector<IR::DpdkAsmStatement> &s);
    const IR::Node *postorder(IR::DpdkListStatement *l) override {
        const IR::IndexedVector<IR::DpdkAsmStatement> *newStmts;
        newStmts = removeJmpAndLabel(l->statements);
        l->statements = *newStmts;
        return l;
    }

    const IR::Node *postorder(IR::DpdkAction *l) override {
        const IR::IndexedVector<IR::DpdkAsmStatement> *newStmts;
        newStmts = removeJmpAndLabel(l->statements);
        l->statements = *newStmts;
        return l;
    }
};

// This pass removes labels whose next instruction is a jmp statement. This pass
// will update any kinds of jmp(conditional + unconditional) that jump to this
// label to the label that jmp statement jump to. For example:
// jeq label1
// ...
// ...
// label1
// jmp label2
//
// will become:
// jeq label2
// ...
// ...
// jmp label2

class ThreadJumps : public Transform {
 public:
    const IR::IndexedVector<IR::DpdkAsmStatement> *threadJumps(
             const IR::IndexedVector<IR::DpdkAsmStatement> &s);

    const IR::Node *postorder(IR::DpdkListStatement *l) override {
        const IR::IndexedVector<IR::DpdkAsmStatement> *newStmts;
        newStmts = threadJumps(l->statements);
        l->statements = *newStmts;
        return l;
    }

    const IR::Node *postorder(IR::DpdkAction *l) override {
        const IR::IndexedVector<IR::DpdkAsmStatement> *newStmts;
        newStmts = threadJumps(l->statements);
        l->statements = *newStmts;
        return l;
    }
};

// This pass removes labels whose next instruction is a label. In addition, it
// will update any jmp that jump to this label to the label next to it.

class RemoveLabelAfterLabel : public Transform {
 public:
    const IR::IndexedVector<IR::DpdkAsmStatement> *removeLabelAfterLabel(
                  const IR::IndexedVector<IR::DpdkAsmStatement> &s);

    const IR::Node *postorder(IR::DpdkListStatement *l) override {
        const IR::IndexedVector<IR::DpdkAsmStatement> *newStmts;
        newStmts = removeLabelAfterLabel(l->statements);
        l->statements = *newStmts;
        return l;
    }

    const IR::Node *postorder(IR::DpdkAction *l) override {
        const IR::IndexedVector<IR::DpdkAsmStatement> *newStmts;
        newStmts = removeLabelAfterLabel(l->statements);
        l->statements = *newStmts;
        return l;
    }
};


// This pass Collects all metadata struct member used in program
class CollectUsedMetadataField : public Inspector {
    ordered_set<cstring>& used_fields;
 public:
    explicit CollectUsedMetadataField(ordered_set<cstring>& used_fields)
        : used_fields(used_fields) {}
    bool preorder(const IR::Member *m) override {
        // metadata struct field used like m.<field_name> in expressions
        if (m->expr->toString() == "m")
            used_fields.insert(m->member.toString());
        return true;
    }
};

// This pass removes all unused fields from metadata struct
class RemoveUnusedMetadataFields : public Transform {
    ordered_set<cstring>& used_fields;
 public:
    explicit RemoveUnusedMetadataFields(ordered_set<cstring>& used_fields)
        : used_fields(used_fields) {}
    const IR::Node* preorder(IR::DpdkAsmProgram *p) override;
};

// This pass validates that the table keys from Metadata struct fit within 64 bytes including any
// holes between the key fields in metadata.
class ValidateTableKeys : public Inspector {
 public:
    ValidateTableKeys() {}
    bool preorder(const IR::DpdkAsmProgram *p) override;
    bool isMetadataStruct(const IR::Type_Struct *st);
};

// Instructions can only appear in actions and apply block of .spec file.
// All these individual passes work on the actions and apply block of .spec file.
class DpdkAsmOptimization : public PassRepeated {
 private:
 public:
    DpdkAsmOptimization() {
        passes.push_back(new RemoveRedundantLabel);
        auto r = new PassRepeated{new RemoveLabelAfterLabel};
        passes.push_back(r);
        passes.push_back(new RemoveConsecutiveJmpAndLabel);
        passes.push_back(new RemoveRedundantLabel);
        passes.push_back(r);
        passes.push_back(new ThreadJumps);
    }
};

}  // namespace DPDK
#endif  /* BACKENDS_DPDK_DPDKASMOPT_H_ */
