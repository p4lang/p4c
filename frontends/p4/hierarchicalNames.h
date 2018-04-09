/*
Copyright 2017 VMware, Inc.

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

#ifndef _FRONTENDS_P4_HIERARCHICALNAMES_H_
#define _FRONTENDS_P4_HIERARCHICALNAMES_H_

#include "ir/ir.h"

namespace P4 {

/**
 * This transform will adjust the @name annotations on objects
 * to reflect their position in the hierarchy.  Only relative names
 * are adjusted.

@name("cc")
control c() {
   @name("a") action a { }
   @name(".b") action b { }
   @name("t")
   table t {
       key = { expr: exact @name("expr") }
   }
   ...
}

becomes

@name("cc")
control c() {
   @name("cc.a") action a { }
   @name(".b") action b { }
   @name("cc.t")
   table t {
       key = { expr: exact @name("expr") }
   }
   ...
}

This pass should be run after inlining.  It assumes that all
externally-visible objects already have @name annotations -- this is
done by the UniqueNames front-end pass.
*/
class HierarchicalNames : public Transform {
    std::vector<cstring> stack;
 public:
    cstring getName(const IR::IDeclaration* decl);

    HierarchicalNames() { setName("HierarchicalNames"); visitDagOnce = false; }
    const IR::Node* preorder(IR::P4Parser* parser) override
    { stack.push_back(getName(parser)); return parser; }
    const IR::Node* postorder(IR::P4Parser* parser) override
    { stack.pop_back(); return parser; }

    const IR::Node* preorder(IR::P4Control* control) override
    { stack.push_back(getName(control)); return control; }
    const IR::Node* postorder(IR::P4Control* control) override
    { stack.pop_back(); return control; }

    const IR::Node* preorder(IR::P4Table* table) override
    { visit(table->annotations); prune(); return table; }

    const IR::Node* postorder(IR::Annotation* annotation) override;
};

}  // namespace P4


#endif  /* _FRONTENDS_P4_HIERARCHICALNAMES_H_ */
