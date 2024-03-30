/*
Copyright 2024 NVIDIA CORPORATION.

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
#include "ir/ir.h"

template<class THIS>
void IR::ForStatement::visit_children(THIS *self, Visitor &v) {
    v.visit(self->init, "init");
    if (auto *cfv = v.controlFlowVisitor()) {
        ControlFlowVisitor::SaveGlobal outer(*cfv, "-BREAK-", "-CONTINUE-");
        ControlFlowVisitor *top = nullptr;
        do {
            top = &cfv->flow_clone();
            cfv->visit(self->condition, "condition", 1);
            auto &inloop = cfv->flow_clone();
            inloop.visit(self->body, "body");
            inloop.flow_merge_global_from("-CONTINUE-");
            inloop.visit(self->updates, "updates");
            cfv->flow_merge(inloop);
            if (cfv->isUnreachable()) break;
        } while (*cfv != *top);
        cfv->flow_merge_global_from("-BREAK-");
    } else {
        v.visit(self->condition, "condition");
        v.visit(self->body, "body");
        v.visit(self->updates, "updates");
    }
}

void IR::ForStatement::visit_children(Visitor &v) { visit_children(this, v); }
void IR::ForStatement::visit_children(Visitor &v) const { visit_children(this, v); }

template<class THIS>
void IR::ForInStatement::visit_children(THIS *self, Visitor &v) {
    v.visit(self->decl, "decl", 0);
    v.visit(self->collection, "collection", 2);
    if (auto *cfv = v.controlFlowVisitor()) {
        ControlFlowVisitor::SaveGlobal outer(*cfv, "-BREAK-", "-CONTINUE-");
        ControlFlowVisitor *top = nullptr;
        do {
            top = &cfv->flow_clone();
            cfv->visit(self->ref, "ref", 1);
            auto &inloop = cfv->flow_clone();
            inloop.visit(self->body, "body", 3);
            cfv->flow_merge(inloop);
            if (cfv->isUnreachable()) break;
        } while (*cfv != *top);
        cfv->flow_merge_global_from("-BREAK-");
    } else {
        v.visit(self->ref, "ref", 1);
        v.visit(self->body, "body", 3);
    }
}
void IR::ForInStatement::visit_children(Visitor &v) { visit_children(this, v); }
void IR::ForInStatement::visit_children(Visitor &v) const { visit_children(this, v); }

void IR::BreakStatement::visit_children(Visitor &v) const {
    if (auto *cfv = v.controlFlowVisitor()) {
        cfv->flow_merge_global_to("-BREAK-");
        cfv->setUnreachable();
    }
}
void IR::BreakStatement::visit_children(Visitor &v) {
    return const_cast<const IR::BreakStatement *>(this)->visit_children(v);
}

void IR::ContinueStatement::visit_children(Visitor &v) const {
    if (auto *cfv = v.controlFlowVisitor()) {
        cfv->flow_merge_global_to("-CONTINUE-");
        cfv->setUnreachable();
    }
}
void IR::ContinueStatement::visit_children(Visitor &v) {
    return const_cast<const IR::ContinueStatement *>(this)->visit_children(v);
}
