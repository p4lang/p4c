/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
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
