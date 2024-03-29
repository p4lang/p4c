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
    self->Statement::visit_children(v);
    v.visit(self->init, "init");
    v.visit(self->condition, "condition");
    // FIXME -- need visit closure over the body and updates + condition
    v.visit(self->body, "body");
    v.visit(self->updates, "updates");
}

void IR::ForStatement::visit_children(Visitor &v) { visit_children(this, v); }
void IR::ForStatement::visit_children(Visitor &v) const { visit_children(this, v); }

template<class THIS>
void IR::ForInStatement::visit_children(THIS *self, Visitor &v) {
    self->Statement::visit_children(v);
    v.visit(self->decl, "decl");
    v.visit(self->ref, "ref");
    v.visit(self->body, "collection");
    v.visit(self->body, "body");
}

void IR::ForInStatement::visit_children(Visitor &v) { visit_children(this, v); }
void IR::ForInStatement::visit_children(Visitor &v) const { visit_children(this, v); }

