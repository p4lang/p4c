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

template <class THIS>
void IR::ForStatement::visit_children(THIS *self, Visitor &v) {
    v.visit(self->annotations, "annotations");
    v.visit(self->init, "init");
    if (auto *cfv = v.controlFlowVisitor()) {
        ControlFlowVisitor::SaveGlobal outer(*cfv, "-BREAK-"_cs, "-CONTINUE-"_cs);
        ControlFlowVisitor *top = nullptr;
        while (true) {
            top = &cfv->flow_clone();
            cfv->visit(self->condition, "condition", 1000);
            auto &inloop = cfv->flow_clone();
            inloop.visit(self->body, "body");
            inloop.flow_merge_global_from("-CONTINUE-"_cs);
            inloop.visit(self->updates, "updates");
            inloop.flow_merge(*top);
            if (inloop == *top) break;
            cfv->flow_copy(inloop);
        }
        cfv->flow_merge_global_from("-BREAK-"_cs);
    } else {
        /* Since there is a variable number of init statements (0 or more), we
         * don't know what the child index of subsequent children will be.  So we
         * arbitrarily set the child index of "condition" to 1000.  As long as there
         * are fewer than 1000 initializers, they'll be child_index 0-999, condition
         * will be 1000, body will be 1001, and updates will be 1002+.  This is
         * only relevant for passes that want to know which child is currently
         * being visited from the Visitor::Context */
        v.visit(self->condition, "condition", 1000);
        v.visit(self->body, "body");
        v.visit(self->updates, "updates");
    }
}

void IR::ForStatement::visit_children(Visitor &v) { visit_children(this, v); }
void IR::ForStatement::visit_children(Visitor &v) const { visit_children(this, v); }

template <class THIS>
void IR::ForInStatement::visit_children(THIS *self, Visitor &v) {
    v.visit(self->annotations, "annotations");
    v.visit(self->decl, "decl", 0);
    v.visit(self->collection, "collection", 2);
    if (auto *cfv = v.controlFlowVisitor()) {
        ControlFlowVisitor::SaveGlobal outer(*cfv, "-BREAK-"_cs, "-CONTINUE-"_cs);
        ControlFlowVisitor *top = nullptr;
        do {
            top = &cfv->flow_clone();
            cfv->visit(self->ref, "ref", 1);
            cfv->visit(self->body, "body", 3);
            cfv->flow_merge_global_from("-CONTINUE-"_cs);
        } while (*cfv != *top);
        cfv->flow_merge_global_from("-BREAK-"_cs);
    } else {
        v.visit(self->ref, "ref", 1);
        v.visit(self->body, "body", 3);
    }
}
void IR::ForInStatement::visit_children(Visitor &v) { visit_children(this, v); }
void IR::ForInStatement::visit_children(Visitor &v) const { visit_children(this, v); }

void IR::BreakStatement::visit_children(Visitor &v) const {
    if (auto *cfv = v.controlFlowVisitor()) {
        cfv->flow_merge_global_to("-BREAK-"_cs);
        cfv->setUnreachable();
    }
}
void IR::BreakStatement::visit_children(Visitor &v) {
    return const_cast<const IR::BreakStatement *>(this)->visit_children(v);
}

void IR::ContinueStatement::visit_children(Visitor &v) const {
    if (auto *cfv = v.controlFlowVisitor()) {
        cfv->flow_merge_global_to("-CONTINUE-"_cs);
        cfv->setUnreachable();
    }
}
void IR::ContinueStatement::visit_children(Visitor &v) {
    return const_cast<const IR::ContinueStatement *>(this)->visit_children(v);
}
