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

#include <stddef.h>

#include <fstream>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/pass_manager.h"
#include "ir/vector.h"
#include "ir/visitor.h"
#include "lib/big_int_util.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"
#include "lib/safe_vector.h"

#define DPDK_TABLE_MAX_KEY_SIZE 64 * 8

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
    ordered_set<cstring> &used_fields;

 public:
    explicit CollectUsedMetadataField(ordered_set<cstring> &used_fields)
        : used_fields(used_fields) {}
    bool preorder(const IR::Member *m) override {
        // metadata struct field used like m.<field_name> in expressions
        if (m->expr->toString() == "m") used_fields.insert(m->member.toString());
        return true;
    }
};

// This pass removes all unused fields from metadata struct
class RemoveUnusedMetadataFields : public Transform {
    ordered_set<cstring> &used_fields;

 public:
    explicit RemoveUnusedMetadataFields(ordered_set<cstring> &used_fields)
        : used_fields(used_fields) {}
    const IR::Node *preorder(IR::DpdkAsmProgram *p) override;
    bool isByteSizeField(const IR::Type *field_type);
};

// This pass shorten the Identifier length
class ShortenTokenLength : public Transform {
    ordered_map<cstring, cstring> &newNameMap;
    static size_t count;
    // Currently Dpdk allows Identifier of 63 char long or less
    // including dots(.) for member exp.
    // worst case member expression will look like below(for headers)
    // 1.30.30 => 63(including dot(.))
    // if id name less than allowedLength keep it same
    cstring shortenString(cstring str, size_t allowedLength = 60) {
        if (str.size() <= allowedLength) return str;
        auto itr = newNameMap.find(str);
        if (itr != newNameMap.end()) return itr->second;
        // make sure new string length less or equal allowedLength
        cstring newStr = str.substr(0, allowedLength - std::to_string(count).size());
        newStr += std::to_string(count);
        count++;
        newNameMap.insert(std::pair<cstring, cstring>(str, newStr));
        origNameMap.insert(std::pair<cstring, cstring>(newStr, str));
        return newStr;
    }

    cstring dropSuffixIfNoAction(IR::ID name) {
        if (name.originalName == "NoAction") return name.originalName;
        return name.name;
    }

 public:
    explicit ShortenTokenLength(ordered_map<cstring, cstring> &newNameMap)
        : newNameMap(newNameMap) {}
    static ordered_map<cstring, cstring> origNameMap;

    const IR::Node *preorder(IR::Member *m) override {
        if (m->toString().startsWith("m.") || m->toString().startsWith("t."))
            m->member = shortenString(m->member);
        else
            m->member = shortenString(m->member, 30);
        return m;
    }

    const IR::Node *preorder(IR::DpdkStructType *s) override {
        if (s->getAnnotations()->getSingle("__packet_data__")) {
            s->name = shortenString(s->name);
            IR::IndexedVector<IR::StructField> changedFields;
            for (auto field : s->fields) {
                IR::StructField *f = new IR::StructField(field->name, field->type);
                f->name = shortenString(f->name, 30);
                changedFields.push_back(f);
            }
            return new IR::DpdkStructType(s->srcInfo, s->name, s->annotations, changedFields);
        } else {
            s->name = shortenString(s->name);
            IR::IndexedVector<IR::StructField> changedFields;
            for (auto field : s->fields) {
                IR::StructField *f = new IR::StructField(field->name, field->type);
                f->name = shortenString(f->name);
                changedFields.push_back(f);
            }
            return new IR::DpdkStructType(s->srcInfo, s->name, s->annotations, changedFields);
        }
        return s;
    }

    const IR::Node *preorder(IR::DpdkHeaderType *h) override {
        h->name = shortenString(h->name);
        IR::IndexedVector<IR::StructField> changedFields;
        for (auto field : h->fields) {
            IR::StructField *f = new IR::StructField(field->name, field->type);
            f->name = shortenString(f->name, 30);
            changedFields.push_back(f);
        }
        return new IR::DpdkHeaderType(h->srcInfo, h->name, h->annotations, changedFields);
    }

    const IR::Node *preorder(IR::DpdkExternDeclaration *e) override {
        e->name = shortenString(e->name);
        return e;
    }

    const IR::Node *preorder(IR::Declaration *g) override {
        g->name = shortenString(g->name);
        return g;
    }

    void shortenParamTypeName(IR::ParameterList &pl) {
        IR::IndexedVector<IR::Parameter> new_pl;
        for (auto p : pl.parameters) {
            auto newType0 = p->type->to<IR::Type_Name>();
            auto path0 = newType0->path->clone();
            path0->name = shortenString(path0->name);
            new_pl.push_back(new IR::Parameter(p->srcInfo, p->name, p->annotations, p->direction,
                                               new IR::Type_Name(newType0->srcInfo, path0),
                                               p->defaultValue));
        }
        pl = IR::ParameterList{new_pl};
    }

    const IR::Node *preorder(IR::DpdkAction *a) override {
        a->name = shortenString(dropSuffixIfNoAction(a->name));
        shortenParamTypeName(a->para);
        return a;
    }

    const IR::Node *preorder(IR::ActionList *al) override {
        IR::IndexedVector<IR::ActionListElement> new_al;
        for (auto ale : al->actionList) {
            auto methodCallExpr = ale->expression->to<IR::MethodCallExpression>();
            auto pathExpr = methodCallExpr->method->to<IR::PathExpression>();
            auto path0 = pathExpr->path->clone();
            path0->name = shortenString(dropSuffixIfNoAction(path0->name));
            new_al.push_back(new IR::ActionListElement(
                ale->srcInfo, ale->annotations,
                new IR::MethodCallExpression(
                    methodCallExpr->srcInfo, methodCallExpr->type,
                    new IR::PathExpression(pathExpr->srcInfo, pathExpr->type, path0),
                    methodCallExpr->typeArguments, methodCallExpr->arguments)));
        }
        return new IR::ActionList(al->srcInfo, new_al);
    }

    const IR::Node *preorder(IR::DpdkTable *t) override {
        t->name = shortenString(t->name);
        auto methodCallExpr = t->default_action->to<IR::MethodCallExpression>();
        auto pathExpr = methodCallExpr->method->to<IR::PathExpression>();
        auto path0 = pathExpr->path->clone();
        path0->name = shortenString(dropSuffixIfNoAction(path0->name));
        t->default_action = new IR::MethodCallExpression(
            methodCallExpr->srcInfo, methodCallExpr->type,
            new IR::PathExpression(pathExpr->srcInfo, pathExpr->type, path0),
            methodCallExpr->typeArguments, methodCallExpr->arguments);
        return t;
    }

    const IR::Node *preorder(IR::DpdkLearner *l) override {
        l->name = shortenString(l->name);
        return l;
    }

    const IR::Node *preorder(IR::DpdkSelector *s) override {
        s->name = shortenString(s->name);
        return s;
    }

    const IR::Node *preorder(IR::DpdkLearnStatement *ls) override {
        ls->action = shortenString(dropSuffixIfNoAction(ls->action));
        return ls;
    }

    const IR::Node *preorder(IR::DpdkApplyStatement *as) override {
        as->table = shortenString(as->table);
        return as;
    }

    const IR::Node *preorder(IR::DpdkJmpStatement *j) override {
        j->label = shortenString(j->label);
        return j;
    }

    const IR::Node *preorder(IR::DpdkLabelStatement *ls) override {
        ls->label = shortenString(ls->label);
        return ls;
    }

    const IR::Node *preorder(IR::DpdkJmpActionStatement *jas) override {
        jas->action = shortenString(dropSuffixIfNoAction(jas->action));
        return jas;
    }
};

/// This pass collect use def info by analysing all possible
/// source and destinations, this info will be used by copy elimination pass
class CollectUseDefInfo : public Inspector {
    P4::TypeMap *typeMap;

 public:
    // Member expression as string considered key for all below maps, and value contains
    // number of occurences either at use or def point.
    // And this point it's assumed all keys are unique.
    std::unordered_map<cstring /*member expresion as string */, int> usesInfo;
    std::unordered_map<cstring, int> defInfo;
    std::unordered_map<cstring /*def*/, const IR::Expression * /*use*/> replacementMap;
    std::unordered_map<cstring, bool> dontEliminate;

    explicit CollectUseDefInfo(P4::TypeMap *typeMap) : typeMap(typeMap) {
        dontEliminate["m.pna_main_output_metadata_output_port"] = true;
        dontEliminate["m.psa_ingress_output_metadata_drop"] = true;
        dontEliminate["m.psa_ingress_output_metadata_egress_port"] = true;
    }

    bool preorder(const IR::DpdkJmpCondStatement *b) override {
        usesInfo[b->src1->toString()]++;
        usesInfo[b->src2->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkLearnStatement *b) override {
        usesInfo[b->timeout->toString()]++;
        dontEliminate[b->timeout->toString()] = true;
        if (b->argument) {
            usesInfo[b->argument->toString()]++;
            // dpdk expect all action argument to be contiguous starting from first argument
            // passed to learner action
            dontEliminate[b->argument->toString()] = true;
        }
        return false;
    }

    bool preorder(const IR::DpdkUnaryStatement *u) override {
        usesInfo[u->src->toString()]++;
        defInfo[u->dst->toString()]++;
        // do not eliminate the destination
        dontEliminate[u->dst->toString()] = true;
        return false;
    }

    bool preorder(const IR::DpdkBinaryStatement *b) override {
        usesInfo[b->src1->toString()]++;
        usesInfo[b->src2->toString()]++;
        defInfo[b->dst->toString()]++;
        // dst and src1 can not be eliminated, because both are same
        // and dpdk does not allow src1 to be constant
        dontEliminate[b->dst->toString()] = true;
        dontEliminate[b->src1->toString()] = true;
        return false;
    }

    bool preorder(const IR::DpdkMovStatement *mv) override {
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

    bool preorder(const IR::DpdkMirrorStatement *m) override {
        usesInfo[m->slotId->toString()]++;
        usesInfo[m->sessionId->toString()]++;
        // dpdk expect it as metadata struct member
        dontEliminate[m->slotId->toString()] = true;
        dontEliminate[m->sessionId->toString()] = true;
        return false;
    }

    bool preorder(const IR::DpdkEmitStatement *e) override {
        auto type = typeMap->getType(e->header)->to<IR::Type_Header>();
        if (type)
            for (auto f : type->fields) {
                cstring name = e->header->toString() + "." + f->name.toString();
                usesInfo[name]++;
            }
        return false;
    }

    bool preorder(const IR::DpdkExtractStatement *e) override {
        auto type = typeMap->getType(e->header)->to<IR::Type_Header>();
        if (type)
            for (auto f : type->fields) {
                cstring name = e->header->toString() + "." + f->name.toString();
                defInfo[name]++;
            }
        if (e->length) {
            usesInfo[e->length->toString()]++;
            // dpdk expect length to be metadata struct member
            dontEliminate[e->length->toString()] = true;
        }
        return false;
    }

    bool preorder(const IR::DpdkLookaheadStatement *l) override {
        auto type = typeMap->getType(l->header)->to<IR::Type_Header>();
        if (type)
            for (auto f : type->fields) {
                cstring name = l->header->toString() + "." + f->name.toString();
                defInfo[name]++;
            }
        return false;
    }

    bool preorder(const IR::DpdkRxStatement *r) override {
        usesInfo[r->port->toString()]++;
        // always required
        dontEliminate[r->port->toString()] = true;
        return false;
    }

    bool preorder(const IR::DpdkTxStatement *t) override {
        usesInfo[t->port->toString()]++;
        // always required
        dontEliminate[t->port->toString()] = true;
        return false;
    }

    bool preorder(const IR::DpdkRecircidStatement *t) override {
        usesInfo[t->pass->toString()]++;
        // uses standard metadata fields
        dontEliminate[t->pass->toString()] = true;
        return false;
    }

    bool preorder(const IR::DpdkRearmStatement *r) override {
        if (r->timeout) {
            usesInfo[r->timeout->toString()]++;
            // dpdk requires it in metadata struct
            dontEliminate[r->timeout->toString()] = true;
        }
        return false;
    }

    bool preorder(const IR::DpdkChecksumAddStatement *c) override {
        usesInfo[c->field->toString()]++;
        // dpdk requires it in header
        if (auto m = c->field->to<IR::Member>())
            if (m->expr->is<IR::Type_Header>()) dontEliminate[c->field->toString()] = true;
        return false;
    }

    bool preorder(const IR::DpdkChecksumSubStatement *c) override {
        usesInfo[c->field->toString()]++;
        // dpdk requires it in header
        if (auto m = c->field->to<IR::Member>())
            if (m->expr->is<IR::Type_Header>()) dontEliminate[c->field->toString()] = true;
        return false;
    }

    bool preorder(const IR::DpdkGetHashStatement *c) override {
        usesInfo[c->dst->toString()]++;
        // dpdk requires it in metadata struct
        dontEliminate[c->dst->toString()] = true;
        return false;
    }

    bool preorder(const IR::DpdkVerifyStatement *v) override {
        usesInfo[v->condition->toString()]++;
        usesInfo[v->error->toString()]++;
        // dpdk requires it in metadata struct
        dontEliminate[v->condition->toString()] = true;
        dontEliminate[v->error->toString()] = true;
        return false;
    }

    bool preorder(const IR::DpdkMeterDeclStatement *c) override {
        usesInfo[c->size->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkMeterExecuteStatement *e) override {
        usesInfo[e->index->toString()]++;
        if (e->length) usesInfo[e->length->toString()]++;
        usesInfo[e->color_in->toString()]++;
        usesInfo[e->color_out->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkCounterCountStatement *c) override {
        usesInfo[c->index->toString()]++;
        if (c->incr) usesInfo[c->incr->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkRegisterDeclStatement *r) override {
        usesInfo[r->size->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkRegisterReadStatement *r) override {
        usesInfo[r->index->toString()]++;
        defInfo[r->dst->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkRegisterWriteStatement *r) override {
        usesInfo[r->index->toString()]++;
        return false;
    }

    bool preorder(const IR::DpdkTable *t) override {
        auto keys = t->match_keys;
        if (keys)
            for (auto ke : keys->keyElements) {
                dontEliminate[ke->expression->toString()] = true;
            }
        return false;
    }

    bool haveSingleUseDef(cstring str) { return defInfo[str] == 1 && usesInfo[str] == 1; }
};

// This pass identifies redundant copies/moves and eliminates them.
class CopyPropagationAndElimination : public Transform {
    std::unordered_map<cstring, int> newUsesInfo;
    P4::TypeMap *typeMap;
    CollectUseDefInfo *collectUseDef;

 public:
    explicit CopyPropagationAndElimination(P4::TypeMap *typeMap) : typeMap(typeMap) {}

    const IR::Expression *getIrreplaceableExpr(cstring str, bool allowConst);
    const IR::Expression *replaceIfCopy(const IR::Expression *expr, bool allowConst = true);
    const IR::DpdkAsmStatement *elimCastOrMov(const IR::DpdkAsmStatement *stmt);
    IR::IndexedVector<IR::DpdkAsmStatement> copyPropAndDeadCodeElim(
        IR::IndexedVector<IR::DpdkAsmStatement> stmts);

    CollectUseDefInfo *calculateUseDef() {
        collectUseDef = new CollectUseDefInfo(typeMap);
        collectUseDef->setCalledBy(this);
        return collectUseDef;
    }
    const IR::Node *preorder(IR::DpdkAction *a) override {
        a->apply(*calculateUseDef());
        return a;
    }

    const IR::Node *preorder(IR::DpdkListStatement *l) override {
        l->apply(*calculateUseDef());
        return l;
    }

    const IR::Node *postorder(IR::DpdkAction *a) override {
        a->statements = copyPropAndDeadCodeElim(a->statements);
        return a;
    }

    const IR::Node *postorder(IR::DpdkListStatement *l) override {
        return new IR::DpdkListStatement(copyPropAndDeadCodeElim(l->statements));
    }
};

// This Pass emits Table config consumed by dpdk target in a text file if
// const entries are present in p4 program.
// Most of the code taken from control-plane/p4RuntimeSerializer.h/.cpp
class EmitDpdkTableConfig : public Inspector {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    ordered_map<cstring, cstring> &newNameMap;
    std::ofstream dpdkTableConfigFile;

    void addExact(const IR::Expression *k, int keyWidth, P4::TypeMap *typeMap);
    void addLpm(const IR::Expression *k, int keyWidth, P4::TypeMap *typeMap);
    void addTernary(const IR::Expression *k, int keyWidth, P4::TypeMap *typeMap);
    void addRange(const IR::Expression *k, int keyWidth, P4::TypeMap *typeMap);
    void addOptional(const IR::Expression *k, int keyWidth, P4::TypeMap *typeMap);
    void addMatchKey(const IR::DpdkTable *table, const IR::ListExpression *keyset,
                     P4::TypeMap *typeMap);
    void addAction(const IR::Expression *actionRef, P4::ReferenceMap *refMap, P4::TypeMap *typeMap);
    int getTypeWidth(const IR::Type *type, P4::TypeMap *typeMap);
    cstring getKeyMatchType(const IR::KeyElement *ke, P4::ReferenceMap *refMap);
    const IR::EntriesList *getEntries(const IR::DpdkTable *dt);
    const IR::Key *getKey(const IR::DpdkTable *dt);
    big_int convertSimpleKeyExpressionToBigInt(const IR::Expression *k, int keyWidth,
                                               P4::TypeMap *typeMap);
    bool tableNeedsPriority(const IR::DpdkTable *table, P4::ReferenceMap *refMap);
    bool isAllKeysDefaultExpression(const IR::ListExpression *keyset);
    void print(cstring str, cstring sep = "");
    void print(big_int, cstring sep = "");

 public:
    EmitDpdkTableConfig(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                        ordered_map<cstring, cstring> &newNameMap)
        : refMap(refMap), typeMap(typeMap), newNameMap(newNameMap) {}
    void postorder(const IR::DpdkTable *table) override;
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
#endif /* BACKENDS_DPDK_DPDKASMOPT_H_ */
