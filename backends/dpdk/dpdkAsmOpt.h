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
#include "dpdkUtils.h"

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

/// This pass collect use def info by analysing all possible
/// source and destinations, this info will be used by copy elimination pass
class CollectUseDefInfo : public Inspector {
    P4::TypeMap* typeMap;

 public:
    // Member expression as string considered key for all below maps, and value contains
    // number of occurences either at use or def point.
    // And this point it's assumed all keys are unique.
    std::unordered_map<cstring /*member expresion as string */, int> usesInfo;
    std::unordered_map<cstring, int> defInfo;
    std::unordered_map<cstring /*def*/, const IR::Expression* /*use*/> replacementMap;
    std::set<cstring> dontEliminate;

    explicit CollectUseDefInfo(P4::TypeMap* typeMap) : typeMap(typeMap) {
        dontEliminate.insert("m.pna_main_output_metadata_output_port");
        dontEliminate.insert("m.psa_ingress_output_metadata_drop");
        dontEliminate.insert("m.psa_ingress_output_metadata_egress_port");
    }

    bool preorder(const IR::DpdkJmpCondStatement *b) override {
        usesInfo[b->src1->toString()]++;
        usesInfo[b->src2->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkLearnStatement *b) override {
        usesInfo[b->timeout->toString()]++;
        if (b->argument)
            usesInfo[b->argument->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkUnaryStatement *u) override {
        usesInfo[u->src->toString()]++;
        defInfo[u->dst->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkBinaryStatement *b) override {
        usesInfo[b->src1->toString()]++;
        usesInfo[b->src2->toString()]++;
        defInfo[b->dst->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkMovStatement *mv) override{
        defInfo[mv->dst->toString()]++;
        usesInfo[mv->src->toString()]++;
        replacementMap[mv->dst->toString()] = mv->src;
        return false;
    }

    bool preorder(const IR::DpdkCastStatement *c) override {
        usesInfo[c->src->toString()]++;
        defInfo[c->dst->toString()]++;
        replacementMap[c->dst->toString()] = c->src;
        return false;
    }

    bool preorder(const IR::DpdkEmitStatement* e) override {
        auto type = typeMap->getType(e->header)->to<IR::Type_Header>();
        if (type)
            for (auto f : type->fields) {
                cstring name = e->header->toString() + "." + f->name.toString();
                usesInfo[name]++;
            }
        return false;
    }

    bool preorder(const IR::DpdkExtractStatement* e) override {
        auto type = typeMap->getType(e->header)->to<IR::Type_Header>();
        if (type)
            for (auto f : type->fields) {
                cstring name = e->header->toString() + "." + f->name.toString();
                defInfo[name]++;
            }
        if (e->length)
            usesInfo[e->length->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkLookaheadStatement* l) override {
        auto type = typeMap->getType(l->header)->to<IR::Type_Header>();
        if (type)
            for (auto f : type->fields) {
                cstring name = l->header->toString() + "." +f->name.toString();
                defInfo[name]++;
            }
        return false;
    }

    bool preorder(const IR::DpdkRxStatement* r) override {
        usesInfo[r->port->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkTxStatement* t) override {
        usesInfo[t->port->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkRecircidStatement* t) override {
        usesInfo[t->pass->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkRearmStatement* r) override {
        if (r->timeout)
            usesInfo[r->timeout->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkChecksumAddStatement* c) override {
        usesInfo[c->field->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkChecksumSubStatement* c) override {
        usesInfo[c->field->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkGetHashStatement* c) override {
        usesInfo[c->dst->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkVerifyStatement* v) override {
        usesInfo[v->condition->toString()]++;
        usesInfo[v->error->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkMeterDeclStatement* c) override {
        usesInfo[c->size->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkMeterExecuteStatement* e) override {
        usesInfo[e->index->toString()]++;
        if (e->length)
            usesInfo[e->length->toString()]++;
        usesInfo[e->color_in->toString()]++;
        usesInfo[e->color_out->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkCounterCountStatement* c) override {
        usesInfo[c->index->toString()]++;
        if (c->incr)
            usesInfo[c->incr->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkRegisterDeclStatement* r) override {
        usesInfo[r->size->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkRegisterReadStatement* r) override {
        usesInfo[r->index->toString()]++;
        defInfo[r->dst->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkRegisterWriteStatement* r) override {
        usesInfo[r->index->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkTable *t) override {
        auto keys = t->match_keys;
        if (keys)
        for (auto ke : keys->keyElements) {
            dontEliminate.insert(ke->expression->toString());
        }
        return false;
    }

    bool haveSingleUseDef(cstring str) {
        return defInfo[str] == 1 && usesInfo[str] == 1;
    }
};

// This pass identifies redundant copies/moves and eliminates them.
class CopyPropagationAndElimination : public Transform {
    std::unordered_map<cstring, int> newUsesInfo;
    P4::TypeMap* typeMap;
    CollectUseDefInfo* collectUseDef;

 public:
    explicit CopyPropagationAndElimination(P4::TypeMap* typeMap) : typeMap(typeMap) {
    }

    const IR::Expression* getIrreplaceableExpr(cstring str, bool allowConst);
    const IR::Expression* replaceIfCopy(const IR::Expression *expr, bool allowConst = true);
    const IR::DpdkAsmStatement* elimCastOrMov(const IR::DpdkAsmStatement* stmt);
    IR::IndexedVector<IR::DpdkAsmStatement> copyPropAndDeadCodeElim(
                                                IR::IndexedVector<IR::DpdkAsmStatement> stmts);

    const IR::Node* preorder(IR::DpdkAction* a) override {
        collectUseDef = new CollectUseDefInfo(typeMap);
        collectUseDef->setCalledBy(this);
        a->apply(*collectUseDef);
        return a;
    }

    const IR::Node* preorder(IR::DpdkListStatement *l) override {
        collectUseDef = new CollectUseDefInfo(typeMap);
        collectUseDef->setCalledBy(this);
        l->apply(*collectUseDef);
        return l;
    }

    const IR::Node* postorder(IR::DpdkAction* a) override {
        a->statements = copyPropAndDeadCodeElim(a->statements);
        return a;
    }

    const IR::Node* postorder(IR::DpdkListStatement *l) override {
        return new IR::DpdkListStatement(copyPropAndDeadCodeElim(l->statements));
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
