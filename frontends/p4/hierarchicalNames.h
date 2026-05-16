/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_HIERARCHICALNAMES_H_
#define FRONTENDS_P4_HIERARCHICALNAMES_H_

#include "ir/ir.h"
#include "ir/visitor.h"

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
class HierarchicalNames : public Modifier {
    std::vector<cstring> stack;

    cstring getName(const IR::IDeclaration *decl) const { return decl->getName(); }

 public:
    HierarchicalNames() {
        setName("HierarchicalNames");
        visitDagOnce = false;
    }
    bool preorder(IR::P4Parser *parser) override {
        stack.push_back(getName(parser));
        return true;
    }
    void postorder(IR::P4Parser *) override { stack.pop_back(); }

    bool preorder(IR::P4Control *control) override {
        stack.push_back(getName(control));
        return true;
    }
    void postorder(IR::P4Control *) override { stack.pop_back(); }

    bool preorder(IR::P4Table *table) override {
        visit(table->annotations);
        return false;
    }

    void postorder(IR::Annotation *annotation) override;
    // Do not change name annotations on parameters
    bool preorder(IR::Parameter *) override { return false; }
};

}  // namespace P4

#endif /* FRONTENDS_P4_HIERARCHICALNAMES_H_ */
