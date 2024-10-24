/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bf-p4c/midend/normalize_params.h"

/** Checks whether any path expression name has @name as its root.  This is
 * useful for determining whether adding/changing a name in a given scope will
 * conflict with existing names.
 */
class CheckForNamePrefix : public Inspector {
    cstring name;
    const IR::Node *node;
    bool preorder(const IR::PathExpression *e) override {
        if (e->path->name.name.startsWith(name.string())) node = e;
        return false;
    }

 public:
    explicit CheckForNamePrefix(cstring name) : name(name), node(nullptr) {}

    /// @returns the first node with CheckForNamePrefix::name as its prefix, or
    /// nullptr if no such name exists.
    const IR::Node *result() const { return node; }
};

/// Renames a parameter and all uses of that parameter.
class RenameParam : public Modifier {
    cstring origName;
    cstring newName;

    /// Maps old path nodes to the new nodes replacing them.
    std::map<const IR::Path *, const IR::Path *> oldToNewPaths;

    /// Maps old parameters to the new nodes replacing them.
    std::map<const IR::IDeclaration *, const IR::Parameter *> oldToNewDecls;

    /// Tracks parameters that have been renamed.  This is equivalent to
    /// oldToNewParams, but it must be distinct because the refMap
    /// operates over IR::Declaration pointers but the typeMap uses IR::Node
    /// pointers.
    std::map<const IR::Parameter *, const IR::Parameter *> oldToNewParams;

    bool preorder(IR::Parameter *param) override {
        if (param->name.name == origName) param->name.name = newName;
        return false;
    }

    bool preorder(IR::Path *p) override {
        if (p->name.name == origName) p->name.name = newName;
        return false;
    }

 public:
    RenameParam(cstring origName, cstring newName) : origName(origName), newName(newName) {}
};

Modifier::profile_t NormalizeParams::init_apply(const IR::Node *root) {
    // TODO: Get architecture-supplied types and corresponding
    // user-supplied instantiations.
    return Modifier::init_apply(root);
}

bool NormalizeParams::preorder(IR::P4Parser *) {
    // TODO: For any architecture parameter names that don't match the
    // user-supplied parameter names, (a) check whether any other instances
    // happen to use the architecture parameter name (and if so, rename them),
    // and then (b) rename the user-supplied parameter name to use the
    // architecture parameter name.
    return false;
}

bool NormalizeParams::preorder(IR::P4Control *) {
    // TODO: For any architecture parameter names that don't match the
    // user-supplied parameter names, (a) check whether any other instances
    // happen to use the architecture parameter name (and if so, rename them),
    // and then (b) rename the user-supplied parameter name to use the
    // architecture parameter name.
    return false;
}
