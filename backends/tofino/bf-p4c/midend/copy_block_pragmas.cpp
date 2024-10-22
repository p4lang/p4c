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

#include "copy_block_pragmas.h"

#include "frontends/p4/methodInstance.h"

class CopyBlockPragmas::FindPragmasFromApply : public Inspector {
    CopyBlockPragmas &self;

    bool preorder(const IR::MethodCallExpression *mc) {
        auto mi = P4::MethodInstance::resolve(mc, self.refMap, self.typeMap, true);
        if (mi && mi->isApply()) {
            if (auto *table = mi->object->to<IR::P4Table>()) {
                const Visitor::Context *ctxt = nullptr;
                while (auto blk = findContext<IR::BlockStatement>(ctxt))
                    for (auto *annot : blk->annotations->annotations)
                        if (self.pragmas.count(annot->name) &&
                            !self.toAdd[table].count(annot->name))
                            self.toAdd[table][annot->name] = annot;
            }
        }
        return false;
    }

 public:
    explicit FindPragmasFromApply(CopyBlockPragmas &self) : self(self) {}
};

class CopyBlockPragmas::CopyToTables : public Modifier {
    CopyBlockPragmas &self;

    bool preorder(IR::P4Table *table) {
        for (auto *annot : Values(self.toAdd[getOriginal<IR::P4Table>()]))
            if (!table->annotations->getSingle(annot->name))
                table->annotations = table->annotations->add(annot);
        return false;
    }

 public:
    explicit CopyToTables(CopyBlockPragmas &self) : self(self) {}
};

CopyBlockPragmas::CopyBlockPragmas(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                                   P4::TypeChecking *typeChecking, std::set<cstring> pragmas)
    : refMap(refMap), typeMap(typeMap), pragmas(pragmas) {
    addPasses({typeChecking, new FindPragmasFromApply(*this), new CopyToTables(*this)});
}
