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
    bool isByteSizeField(const IR::Type *field_type);
};

// This pass validates that the table keys from Metadata struct fit within 64 bytes including any
// holes between the key fields in metadata.
class ValidateTableKeys : public Inspector {
 public:
    ValidateTableKeys() {}
    bool preorder(const IR::DpdkAsmProgram *p) override;
    bool isMetadataStruct(const IR::Type_Struct *st);
    int getFieldSizeBits(const IR::Type *field_type);
};

// This pass shorten the Identifier length
class ShortenTokenLength : public Transform {
    ordered_map<cstring, cstring> newNameMap;
    static size_t count;
    // Currently Dpdk allows Identifier of 63 char long or less
    // including dots(.) for member exp.
    // worst case member expression will look like below(for headers)
    // 1.30.30 => 63(including dot(.))
    // if id name less than allowedLength keep it same
    cstring shortenString(cstring str, size_t allowedLength = 60) {
        if (str.size() <= allowedLength)
            return str;
         auto itr = newNameMap.find(str);
         if (itr != newNameMap.end())
             return itr->second;
         // make sure new string length less or equal allowedLength
         cstring newStr = str.substr(0, allowedLength - std::to_string(count).size());
         newStr += std::to_string(count);
         count++;
         newNameMap.insert(std::pair<cstring, cstring>(str, newStr));
         origNameMap.insert(std::pair<cstring, cstring>(newStr, str));
         return newStr;
    }

 public:
    static ordered_map<cstring, cstring> origNameMap;

    const IR::Node* preorder(IR::Member *m) override {
        if (m->toString().startsWith("m."))
            m->member = shortenString(m->member);
        else
            m->member = shortenString(m->member, 30);
        return m;
    }

    const IR::Node* preorder(IR::DpdkStructType *s) override {
        if (s->getAnnotations()->getSingle("__packet_data__")) {
            s->name = shortenString(s->name);
            IR::IndexedVector<IR::StructField> changedFields;
            for (auto field : s->fields) {
                IR::StructField *f = new IR::StructField(field->name, field->type);
                f->name = shortenString(f->name, 30);
                changedFields.push_back(f);
            }
            return new IR::DpdkStructType(s->srcInfo, s->name,
                                          s->annotations, changedFields);
        } else {
            s->name = shortenString(s->name);
            IR::IndexedVector<IR::StructField> changedFields;
            for (auto field : s->fields) {
                IR::StructField *f = new IR::StructField(field->name, field->type);
                f->name = shortenString(f->name);
                changedFields.push_back(f);
            }
            return new IR::DpdkStructType(s->srcInfo, s->name,
                                          s->annotations, changedFields);
        }
        return s;
    }

    const IR::Node* preorder(IR::DpdkHeaderType *h) override {
        h->name = shortenString(h->name, 30);
        IR::IndexedVector<IR::StructField> changedFields;
        for (auto field : h->fields) {
             IR::StructField *f = new IR::StructField(field->name, field->type);
             f->name = shortenString(f->name, 30);
             changedFields.push_back(f);
        }
        return new IR::DpdkHeaderType(h->srcInfo, h->name,
                                      h->annotations, changedFields);
    }

    const IR::Node* preorder(IR::DpdkExternDeclaration *e) override {
        e->name = shortenString(e->name);
        return e;
    }

    const IR::Node* preorder(IR::Declaration *g) override {
        g->name = shortenString(g->name);
        return g;
    }

    const IR::Node* preorder(IR::Path *p) override {
        p->name = shortenString(p->name);
        return p;
    }

    const IR::Node* preorder(IR::DpdkAction *a) override {
        a->name = shortenString(a->name);
        return a;
    }

    const IR::Node* preorder(IR::DpdkTable *t) override {
        t->name = shortenString(t->name);
        return t;
    }

    const IR::Node* preorder(IR::DpdkLearner *l) override {
        l->name = shortenString(l->name);
        return l;
    }

    const IR::Node* preorder(IR::DpdkSelector *s) override {
        s->name = shortenString(s->name);
        return s;
    }

    const IR::Node* preorder(IR::DpdkLearnStatement *ls) override{
        ls->action = shortenString(ls->action);
        return ls;
    }

    const IR::Node* preorder(IR::DpdkApplyStatement *as) override{
        as->table = shortenString(as->table);
        return as;
    }
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
