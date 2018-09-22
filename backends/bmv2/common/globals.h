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

#ifndef BACKENDS_BMV2_COMMON_GLOBALS_H_
#define BACKENDS_BMV2_COMMON_GLOBALS_H_

#include "ir/ir.h"
#include "backend.h"

namespace BMV2 {

class ConvertGlobals : public Inspector {
    ConversionContext* ctxt;
    const bool emitExterns;

 public:
    explicit ConvertGlobals(ConversionContext* ctxt, const bool& emitExterns_) :
    ctxt(ctxt), emitExterns(emitExterns_) {
        setName("ConvertGlobals"); }

    bool preorder(const IR::ExternBlock* block) override;
    bool preorder(const IR::ToplevelBlock *block) override {
        /// Blocks are not in IR tree, use a custom visitor to traverse
        for (auto it : block->constantValue) {
            if (it.second->is<IR::Block>())
                visit(it.second->getNode());
        }
        return false;
    }
};

}  // namespace BMV2

#endif /* BACKENDS_BMV2_COMMON_GLOBALS_H_ */
